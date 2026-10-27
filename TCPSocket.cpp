#include "TCPSocket.h"
#include "TempFailure.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>

using namespace NET;

TCPSocket::TCPSocket( AddressFamily family)
: InternetSocket( family, SOCK_STREAM, IPPROTO_TCP)
{}

TCPSocket::TCPSocket( Handle&& handle)
: InternetSocket( handle.release() )
{}

void TCPSocket::setNoDelay( bool enable)
{
	const int value = enable;
	if( ::setsockopt( m_socket, IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value)) < 0)
		throw SocketException("Failed to set TCP_NODELAY (setsockopt)");
}

void TCPSocket::listen( int backlog)
{
	int ret = ::listen( m_socket, backlog);
	if( ret < 0)
		throw SocketException("TCPSocket::listen failed");
}

TCPSocket::Handle TCPSocket::accept() const
{
#ifdef SOCK_CLOEXEC
	// prevent leaking socket descriptor on exec
	int ret = TEMP_FAILURE_RETRY (::accept4( m_socket, nullptr, nullptr, SOCK_CLOEXEC));
#else
	int ret = TEMP_FAILURE_RETRY (::accept( m_socket, nullptr, nullptr));
#endif
	if( ret < 0)
		throw SocketException("TCPSocket::accept failed");

	return Handle(ret);
}

TCPSocket::Handle TCPSocket::timedAccept( int timeout) const
{
	struct pollfd poll;
	poll.fd = m_socket;
	poll.events = POLLIN;

	int ret = TEMP_FAILURE_RETRY (::poll( &poll, 1, timeout));
	if( ret == 0) return Handle();
	if( ret < 0)
		throw SocketException("TCPSocket::timedAccept failed (poll)");

	if( poll.revents & (POLLERR | POLLNVAL))
		throw SocketException("TCPSocket::timedAccept failed (poll)");

	return accept();
}
