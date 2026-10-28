#include "SimpleSocket.h"
#include "SockAddress.h"
#include "TempFailure.h"

#include <poll.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>

using namespace NET;

SocketException::SocketException( std::string_view message, bool inclSysMsg /* = true */)
: m_message(message)
, m_errorcode(0)
{
	if(inclSysMsg)
	{
		m_errorcode = errno;
		m_message += ": ";
		m_message += strerror(m_errorcode);
	}
}

SimpleSocket::SimpleSocket( int domain, int type, int protocol)
: m_peerDisconnected(false)
, m_connectionOriented(type == SOCK_STREAM)
{
#ifdef SOCK_CLOEXEC
	// prevent leaking socket descriptor on exec
	type |= SOCK_CLOEXEC;
#endif
	if( (m_socket = ::socket( domain, type, protocol)) < 0)
		throw SocketException("Socket creation failed (socket)");
}

SimpleSocket::SimpleSocket( int sockfd)
: m_socket(sockfd)
, m_peerDisconnected(false)
, m_connectionOriented(true) // accepted sockets have connections
{
	if(sockfd < 0)
		throw SocketException("Tried to initialize Socket with invalid Handle", false);
}

SimpleSocket::~SimpleSocket()
{
	if( m_socket >= 0) ::close(m_socket);
}

int SimpleSocket::send( const void* buffer, size_t len)
{
	int flags = 0;
#ifdef MSG_NOSIGNAL
	// do not generate SIGPIPE on closed connection
	flags |= MSG_NOSIGNAL;
#endif
	int sent = TEMP_FAILURE_RETRY (::send( m_socket, (const raw_type*) buffer, len, flags));
	if( sent < 0)
	{
		if( errno == ECONNRESET || errno == ECONNREFUSED || errno == EPIPE)
		{
			m_peerDisconnected = true;
			return 0;
		}
		throw SocketException("Send failed (send)");
	}
	return sent;
}

int SimpleSocket::receive( void* buffer, size_t len)
{
	int ret = TEMP_FAILURE_RETRY (::recv( m_socket, (raw_type*) buffer, len, 0));
	if( ret < 0)
	{
		if( errno == ECONNRESET && m_connectionOriented)
		{
			m_peerDisconnected = true;
			return 0;
		}
		throw SocketException("Received failed (receive)");
	}

	if( ret == 0 && len != 0 && m_connectionOriented)
		m_peerDisconnected = true;

	return ret;
}

int SimpleSocket::timedReceive( void* buffer, size_t len, int timeout)
{
	struct pollfd poll;
	poll.fd = m_socket;
	poll.events = POLLIN | POLLPRI | POLLRDHUP;

	int ret = TEMP_FAILURE_RETRY (::poll( &poll, 1, timeout));

	if( ret == 0) return 0;
	if( ret < 0)  throw SocketException("timedReceive failed (poll)");

	if( poll.revents & POLLRDHUP || poll.revents & POLLHUP)
		m_peerDisconnected = true;

	if( poll.revents & POLLIN || poll.revents & POLLPRI)
		return receive( buffer, len);

	if( poll.revents & (POLLERR | POLLNVAL))
		throw SocketException("timedReceive failed (poll)");

	return 0;
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
	m_peerDisconnected = false;
}

void SimpleSocket::shutdown( ShutdownDirection type)
{
	if( ::shutdown( m_socket, type) < 0)
		throw SocketException("Shutdown failed (shutdown)");
}

bool SimpleSocket::peerDisconnected() const noexcept
{
	return m_peerDisconnected;
}

void SimpleSocket::listen( int backlog)
{
	int ret = ::listen( m_socket, backlog);
	if( ret < 0)
		throw SocketException("Listen failed (listen)");
}

int SimpleSocket::accept() const
{
#ifdef SOCK_CLOEXEC
	// prevent leaking socket descriptor on exec
	int ret = TEMP_FAILURE_RETRY (::accept4( m_socket, nullptr, nullptr, SOCK_CLOEXEC));
#else
	int ret = TEMP_FAILURE_RETRY (::accept( m_socket, nullptr, nullptr));
#endif
	if( ret < 0)
		throw SocketException("Accept failed (accept)");

	return ret;
}

int SimpleSocket::timedAccept( int timeout) const
{
	struct pollfd poll;
	poll.fd = m_socket;
	poll.events = POLLIN;

	int ret = TEMP_FAILURE_RETRY (::poll( &poll, 1, timeout));
	if( ret == 0) return -1; // invalid handle
	if( ret < 0)
		throw SocketException("timedAccept failed (poll)");

	if( poll.revents & (POLLERR | POLLNVAL))
		throw SocketException("timedAccept failed (poll)");

	return SimpleSocket::accept();
}

int SimpleSocket::sendAll( const void* buffer, size_t len)
{
	size_t sent = 0;
	while( sent != len)
	{
		const char* buf = static_cast<const char*>(buffer) + sent;
		auto ret = send( buf, len - sent);
		sent += static_cast<size_t>(ret);
		if( m_peerDisconnected) break;
	}
	return sent;
}

int SimpleSocket::receiveAll( void* buffer, size_t len)
{
	size_t received = 0;
	while( received != len)
	{
		char* buf = static_cast<char*>(buffer) + received;
		auto ret = receive( buf, len - received);
		if( ret == 0) break;
		received += static_cast<size_t>(ret);
	}
	return received;
}
