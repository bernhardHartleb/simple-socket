#include "SimpleSocket.h"
#include "SockAddress.h"
#include "TempFailure.h"
#include "PollHelper.h"

#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <chrono>

using namespace NET;

namespace {

class NonBlockingGuard
{
public:
	explicit NonBlockingGuard(int fd) : m_fd(fd)
	{
		m_flags = ::fcntl( m_fd, F_GETFL, 0);
		if( m_flags < 0)
			throw SocketException("Unable to get file status (fcntl)");
		if( ::fcntl( m_fd, F_SETFL, m_flags | O_NONBLOCK) < 0)
			throw SocketException("Unable to set non-blocking mode (fcntl)");
	}

	~NonBlockingGuard()
	{
		const int savedErrno = errno;
		::fcntl( m_fd, F_SETFL, m_flags);
		errno = savedErrno;
	}

	NonBlockingGuard(const NonBlockingGuard&) = delete;
	NonBlockingGuard& operator=(const NonBlockingGuard&) = delete;

private:
	int m_fd;
	int m_flags;
};

}; // namespace


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

int SimpleSocket::duplicateNativeHandle() const
{
	// Atomically duplicate with close-on-exec
	int ret = ::fcntl( m_socket, F_DUPFD_CLOEXEC, 0);
	if( ret < 0)
		throw SocketException("Duplicate failed (fcntl)");

	return ret;
}

bool SimpleSocket::valid() const noexcept
{
	if( m_socket < 0) return false;
	// check the descriptor by reading its flags
	return (::fcntl( m_socket, F_GETFD) >= 0);
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
	pfd.events = POLLRDHUP;

	const int ret = TEMP_FAILURE_RETRY(::poll( &pfd, 1, 0));
	if( ret < 0)
		throw SocketException("peerDisconnected failed (poll)");

	if( ret == 0) return false;

	if( pfd.revents & POLLNVAL)
		throw SocketException("peerDisconnected failed (invalid socket)", false);

	// HUP or error states means the connection is gone
	return pfd.revents & (POLLRDHUP | POLLHUP | POLLERR);
}

bool SimpleSocket::isBlocking() const
{
	int flags = ::fcntl( m_socket, F_GETFL, 0);
	if( flags < 0)
		throw SocketException("Unable to get file status (fcntl)");

	return !(flags & O_NONBLOCK);
}

void SimpleSocket::setBlocking(bool bl)
{
	int flags = ::fcntl( m_socket, F_GETFL, 0);
	if( flags < 0)
		throw SocketException("Unable to get file status (fcntl)");

	if(bl) flags &= ~O_NONBLOCK;
	else   flags |= O_NONBLOCK;

	if( ::fcntl( m_socket, F_SETFL, flags) < 0)
		throw SocketException("Unable to set non-blocking mode (fcntl)");
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
		// non-blocking? return invalid
		if( error == EAGAIN || error == EWOULDBLOCK)
			return -1;

		// error indicates we should try again
		if( error == EINTR || detail::isTransientAcceptError(error))
			continue;

		throw SocketException("Accept failed (accept)", error);
	}
}

int SimpleSocket::timedAccept( int timeout) const
{
	bool firstRun = true;
	using Clock = std::chrono::steady_clock;
	auto now = Clock::now();
	// negative timeout means wait forever
	const bool infinite = timeout < 0;
	const auto deadline = infinite ? Clock::time_point::max()
		: now + std::chrono::milliseconds(timeout);

	while(1)
	{
		// ensure now == deadline when timeout is zero
		if( !firstRun) now = Clock::now();
		// invalid socket handle on timeout
		if( now >= deadline && !firstRun) return -1;
		firstRun = false;

		auto remaining = std::chrono::ceil<std::chrono::milliseconds>(deadline - now);
		const int pollTimeout = static_cast<int>(remaining.count());

		pollfd pfd {};
		pfd.fd = m_socket;
		pfd.events = POLLIN;

		int ret = ::poll( &pfd, 1, pollTimeout);
		if( ret == 0) return -1; // invalid socket handle on timeout
		if( ret < 0)
		{
			if( errno == EINTR) continue;
			throw SocketException("timedAccept failed (poll)");
		}

		if( pfd.revents & POLLNVAL)
			throw SocketException("timedAccept failed (invalid socket)", false);

		if( pfd.revents & POLLIN)
		{
			int fd = acceptNative();
			if( fd >= 0) return fd;

			const int error = errno;
			// try again until timeout
			if( error == EAGAIN || error == EWOULDBLOCK)
				continue;

			// error indicates we should try again
			if( error == EINTR || detail::isTransientAcceptError(error))
				continue;

			throw SocketException("timedAccept failed (accept)", error);
		}

		if( pfd.revents & POLLERR)
			throw SocketException("timedAccept failed (POLLERR)", false);

		if( pfd.revents & POLLHUP)
			throw SocketException("timedAccept failed (listening socket hung up)", false);
	}
}

ConnectResult SimpleSocket::connect( const SockAddress& addr)
{
	if( ::connect( m_socket, addr.data(), addr.length()) == 0)
		return ConnectResult::Connected;

	const int error = errno;
	switch(error)
	{
		case EINPROGRESS:
		case EALREADY:
		case EINTR:
			return ConnectResult::InProgress;
	}

	// Linux uses EGAGIN for unix socckets non-blocking connect
	if( error == EAGAIN && addr.family() == AF_UNIX)
		return ConnectResult::InProgress;

	if( auto result = detail::connectResultFromError(error))
		return *result;

	throw SocketException("Connect failed (connect)", error);
}

ConnectResult SimpleSocket::timedConnect( const SockAddress& addr, int timeout)
{
	// Set O_NONBLOCK and restore blocking state on destruction
	NonBlockingGuard nonBlocking( m_socket);

	// Asynchronous connection attempt
	int ret = ::connect( m_socket, addr.data(), addr.length());
	if( ret == 0) // Already connected
		return ConnectResult::Connected;

	int error = errno;
	// known connection issue?
	auto result = detail::connectResultFromError(error);
	if( result) return *result;

	// Linux uses EGAGIN for unix socckets non-blocking connect
	if( error == EAGAIN && addr.family() == AF_UNIX)
		error = EINPROGRESS;

	// should we continue with poll?
	switch(error)
	{
		case EINPROGRESS: // asynchronous connect active
		case EALREADY:    // previous attempt still active, potentially not to the same endpoint
		case EINTR:
			break;
		default:
			throw SocketException("timedConnect failed (connect)", error);
	}

	// Wait for the connection to complete
	pollfd pfd {};
	pfd.fd = m_socket;
	pfd.events = POLLOUT;

	ret = TEMP_FAILURE_RETRY (::poll( &pfd, 1, timeout));
	if( ret < 0)
		throw SocketException("timedConnect failed (poll)");

	if( ret == 0) return ConnectResult::Timeout;

	if( pfd.revents & POLLNVAL)
		throw SocketException("timedConnect failed (invalid socket)", false);

	// Check for socket errors after poll success
	error = 0;
	socklen_t len = sizeof(error);
	if( ::getsockopt( m_socket, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
		throw SocketException("timedConnect failed (getsockopt)");

	if( error == 0) return ConnectResult::Connected;

	// known connection issues should not throw
	result = detail::connectResultFromError(error);
	if( result) return *result;

	throw SocketException("timedConnect failed", error);
}

IOResult SimpleSocket::sendAll( const void* buffer, size_t len)
{
	size_t sent = 0;
	while( sent < len)
	{
		const char* pos = static_cast<const char*>(buffer) + sent;
		IOResult res = send( pos, len - sent);
		sent += res.bytes;
		if( res.status != IOStatus::Success) return { res.status, sent };
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
		if( res.status != IOStatus::Success) return { res.status, received };
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
