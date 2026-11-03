#include "SimpleSocket.h"
#include "SockAddress.h"
#include "TempFailure.h"
#include "PollHelper.h"

#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <chrono>

using namespace NET;

SocketException::SocketException( std::string_view message, bool inclErrno /* = true */)
: m_message(message)
, m_errorcode(0)
{
	if(inclErrno)
	{
		m_errorcode = errno;
		m_message += ": ";
		m_message += strerror(m_errorcode);
	}
}

SocketException::SocketException( std::string_view message, int error)
: m_message(message)
, m_errorcode(error)
{
	if(error != 0)
	{
		m_message += ": ";
		m_message += strerror(m_errorcode);
	}
}


SimpleSocket::SimpleSocket( int domain, int type, int protocol)
: m_connectionOriented(type == SOCK_STREAM)
{
#ifdef SOCK_CLOEXEC
	// prevent leaking socket descriptor on exec
	type |= SOCK_CLOEXEC;
#endif
	if( (m_socket = ::socket( domain, type, protocol)) < 0)
		throw SocketException("Socket creation failed (socket)");
}

SimpleSocket::SimpleSocket( AdoptSocketTag, int sockfd, int type)
: m_socket(sockfd)
, m_connectionOriented(type == SOCK_STREAM)
{
	if(sockfd < 0)
		throw SocketException("Tried to initialize Socket with invalid Handle", false);
}

SimpleSocket::SimpleSocket( SimpleSocket&& other) noexcept
: m_socket(other.m_socket)
, m_connectionOriented(other.m_connectionOriented)
{
	other.m_socket = -1;
	other.m_connectionOriented = false;
}

SimpleSocket& SimpleSocket::operator=( SimpleSocket&& other) noexcept
{
	if( this != &other)
	{
		if( m_socket >= 0) ::close(m_socket);

		m_socket = other.m_socket;
		m_connectionOriented = other.m_connectionOriented;

		other.m_socket = -1;
		other.m_connectionOriented = false;
	}
	return *this;
}

SimpleSocket::~SimpleSocket()
{
	if( m_socket >= 0) ::close(m_socket);
}

IOResult SimpleSocket::send( const void* buffer, size_t len)
{
	int flags = 0;
#ifdef MSG_NOSIGNAL
	// do not generate SIGPIPE on closed connection
	flags |= MSG_NOSIGNAL;
#endif
	ssize_t ret = TEMP_FAILURE_RETRY (::send( m_socket, (const raw_type*) buffer, len, flags));
	if( ret >= 0)
		return { IOStatus::Success, static_cast<size_t>(ret) };

	auto status = detail::classifyIoError( errno, m_connectionOriented);
	if( status) return IOResult{ *status, 0 };

	throw SocketException("Send failed (send)");
}

IOResult SimpleSocket::receive( void* buffer, size_t len)
{
	ssize_t ret = TEMP_FAILURE_RETRY (::recv( m_socket, (raw_type*) buffer, len, 0));
	if( ret > 0)
		return { IOStatus::Success, static_cast<size_t>(ret) };

	// recv() == 0 means EOF only for connection-oriented sockets
	// for datagram sockets a zero-length is valid
	if( ret == 0)
	{
		if( m_connectionOriented && len != 0)
		{
			return { IOStatus::PeerDisconnected, 0 };
		}
		return { IOStatus::Success, 0 };
	}

	auto status = detail::classifyIoError( errno, m_connectionOriented);
	if( status) return IOResult{ *status, 0 };

	throw SocketException("Receive failed (receive)");
}

IOResult SimpleSocket::timedReceive( void* buffer, size_t len, int timeout)
{
	return detail::timedReceiveImpl( m_socket, m_connectionOriented, timeout,
		[&] { return receive( buffer, len); });
}

void SimpleSocket::disconnect()
{
	sockaddr addr;
	std::memset( &addr, 0, sizeof(addr));
	addr.sa_family = AF_UNSPEC;

	if( ::connect( m_socket, &addr, sizeof(addr)) < 0)
	{
		if( errno != ECONNRESET)
			throw SocketException("Disconnect failed (connect)");
	}
}

void SimpleSocket::shutdown( ShutdownDirection type)
{
	int how = 0;
	switch(type)
	{
	case STOP_RECEIVE: how = SHUT_RD; break;
	case STOP_SEND:    how = SHUT_WR; break;
	case STOP_BOTH:    how = SHUT_RDWR; break;
	}

	if( ::shutdown( m_socket, how) < 0)
	{
		if( errno != ENOTCONN)
			throw SocketException("Shutdown failed (shutdown)");
	}
}

bool SimpleSocket::peerDisconnected() const
{
	if( !m_connectionOriented) return false;

	pollfd pfd {};
	pfd.fd = m_socket;
	pfd.events = POLLIN | POLLRDHUP;

	const int ret = TEMP_FAILURE_RETRY(::poll( &pfd, 1, 0));
	if( ret < 0)
		throw SocketException("peerDisconnected failed (poll)");

	if( ret == 0) return false;

	if( pfd.revents & POLLNVAL)
		throw SocketException("peerDisconnected failed (invalid socket)", false);

	// HUP or error states means the connection is gone
	return pfd.revents & (POLLRDHUP | POLLHUP | POLLERR);
}

void SimpleSocket::listen( int backlog)
{
	int ret = ::listen( m_socket, backlog);
	if( ret < 0)
		throw SocketException("Listen failed (listen)");
}

int SimpleSocket::acceptNative() const
{
#ifdef SOCK_CLOEXEC
	// prevent leaking socket descriptor on exec
	return ::accept4( m_socket, nullptr, nullptr, SOCK_CLOEXEC);
#else
	return ::accept( m_socket, nullptr, nullptr);
#endif
}

int SimpleSocket::accept() const
{
	// Retry until we reach a non-transient error
	while(1)
	{
		int fd = acceptNative();
		if( fd >= 0) return fd;

		const int error = errno;
		if( error == EINTR || detail::isTransientAcceptError(error))
			continue;

		throw SocketException("Accept failed (accept)", error);
	}
}

int SimpleSocket::timedAccept( int timeout) const
{
	// negative timeout means wait forever
	if( timeout < 0) return accept();

	using Clock = std::chrono::steady_clock;
	const auto deadline = Clock::now() + std::chrono::milliseconds(timeout);

	while(1)
	{
		const auto now = Clock::now();
		if( now >= deadline) return -1;

		auto remaining = std::chrono::ceil<std::chrono::milliseconds>(deadline - now);
		const int pollTimeout = static_cast<int>(remaining.count());

		pollfd pfd {};
		pfd.fd = m_socket;
		pfd.events = POLLIN;

		int ret = ::poll( &pfd, 1, pollTimeout);
		if( ret == 0) return -1; // invalid handle on timeout
		if( ret < 0)
		{
			const int error = errno;
			if( error == EINTR) continue;

			throw SocketException("timedAccept failed (poll)", error);
		}

		if( pfd.revents & POLLNVAL)
			throw SocketException("timedAccept failed (invalid socket)", false);

		if( pfd.revents & POLLIN)
		{
			int fd = acceptNative();
			if( fd >= 0) return fd;

			const int error = errno;
			if( error == EINTR
			 || error == EAGAIN || error == EWOULDBLOCK
			 || detail::isTransientAcceptError(error))
				continue;

			throw SocketException("timedAccept failed (accept)", error);
		}

		if( pfd.revents & POLLERR)
			throw SocketException("timedAccept failed (POLLERR)", false);

		if( pfd.revents & POLLHUP)
			throw SocketException("timedAccept failed (listening socket hung up)", false);
	}
}

IOResult SimpleSocket::sendAll( const void* buffer, size_t len)
{
	size_t sent = 0;
	while( sent < len)
	{
		const char* pos = static_cast<const char*>(buffer) + sent;
		IOResult res = send( pos, len - sent);
		sent += res.bytes;
		if( res.bytes == 0) return { res.status, sent };
	}
	return { IOStatus::Success, sent };
}

IOResult SimpleSocket::receiveAll( void* buffer, size_t len)
{
	size_t received = 0;
	while( received < len)
	{
		char* pos = static_cast<char*>(buffer) + received;
		IOResult res = receive( pos, len - received);
		received += res.bytes;
		if( res.bytes == 0) return { res.status, received };
	}
	return { IOStatus::Success, received };
}

void SimpleSocket::validateSocket( int sockfd, int type_req, int protocol_req)
{
	if( sockfd < 0)
		throw SocketException("Invalid native handle", false);

	int type = 0;
	socklen_t len = sizeof(type);
	if( ::getsockopt( sockfd, SOL_SOCKET, SO_TYPE, &type, &len) < 0)
		throw SocketException("Unable to read native socket type (getsockopt)");
	if( type != type_req)
		throw SocketException("Native handle is the wrong socket type", false);

#ifdef SO_PROTOCOL
	int protocol = 0;
	len = sizeof(protocol);
	if( ::getsockopt( sockfd, SOL_SOCKET, SO_PROTOCOL, &protocol, &len) < 0)
		throw SocketException("Unable to read native socket protocol (getsockopt)");
	if( protocol != protocol_req)
		throw SocketException("Native handle uses wrong protocol type", false);
#endif
}
