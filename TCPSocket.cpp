#include "TCPSocket.h"
#include "TempFailure.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>

using namespace NET;

TCPSocket::TCPSocket()
: InternetSocket( STREAM, IPPROTO_TCP)
{}

TCPSocket::TCPSocket( Handle handle)
: InternetSocket( handle.release() )
{}

int TCPSocket::sendAll( const void* buffer, size_t len)
{
	size_t sent = 0;
	while( sent != len)
	{
		const char* buf = static_cast<const char*>(buffer) + sent;
		int ret = send( buf, len - sent);
		if( ret < 0) return ret;
		sent += static_cast<unsigned>(ret);
	}
	return sent;
}

void TCPSocket::listen( int backlog /* = 0 */)
{
	int ret = ::listen( m_socket, backlog);
	if( ret < 0)
		throw SocketException("listen failed, most likely another socket is already listening on the same port");
}

TCPSocket::Handle TCPSocket::accept() const
{
	int ret = ::accept( m_socket, 0, 0);
	if( ret <= 0)
		throw SocketException("TCPSocket::accept failed");
	return Handle(ret);
}

TCPSocket::Handle TCPSocket::timedAccept( int timeout) const
{
	struct pollfd poll;
	poll.fd = m_socket;
	poll.events = POLLIN;

	int ret = TEMP_FAILURE_RETRY (::poll( &poll, 1, timeout));

	if( ret == 0) return Handle(0);
	if( ret < 0) throw SocketException("Poll failed (receive)");

	ret = ::accept( m_socket, 0, 0);
	if( ret <= 0)
		throw SocketException("TCPSocket::timedAccept failed");
	return Handle(ret);
}
