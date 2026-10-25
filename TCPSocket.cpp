#include "TCPSocket.h"
#include "TempFailure.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>

using namespace NET;

TCPSocket::TCPSocket()
: InternetSocket( STREAM, IPPROTO_TCP)
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
	int ret = TEMP_FAILURE_RETRY (::accept( m_socket, nullptr, nullptr));
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
