#include "TCPSocket.h"

#include <poll.h>
#include <sys/types.h>		// for data types
#include <netinet/in.h>		// for sockaddr_in

using namespace NET;

TCPSocket::TCPSocket()
: InternetSocket( STREAM, IPPROTO_TCP)
{}

TCPSocket::TCPSocket( const std::string& foreignAddress, unsigned short foreignPort)
: InternetSocket( STREAM, IPPROTO_TCP)
{
	connect( foreignAddress, foreignPort);
}

TCPSocket::TCPSocket( Handle handle)
: InternetSocket( handle.m_sockfd)
{
	if(!handle) throw SocketException("Tried to initialize TCPSocket with invalid Handle", false);
}

int TCPSocket::sendAll( const void* buffer, size_t len)
{
	size_t sent = 0;
	while(sent != len)
	{
		const char* buf = static_cast<const char*>(buffer) + sent;

		int ret = ::send( m_socket, (raw_type*) buf, len - sent, 0);
		if( ret < 0)
			throw SocketException("Send failed (send)");

		sent += static_cast<unsigned>(ret);
		if(sent == len) break;
	}
	return sent;
}

void TCPSocket::listen( int backlog /* = 0 */)
{
	int ret = ::listen(m_socket, backlog);
	if( ret < 0)
		throw SocketException("listen failed, most likely another socket is already listening on the same port");
}

TCPSocket::Handle TCPSocket::accept() const
{
	int ret;
	if( (ret = ::accept( m_socket, 0, 0)) <= 0)
		throw SocketException("TCPSocket::accept failed");
	return Handle(ret);
}

TCPSocket::Handle TCPSocket::timedAccept( int timeout) const
{
	struct pollfd poll;
	poll.fd = m_socket;
	poll.events = POLLIN;

	int ret = ::poll( &poll, 1, timeout);

	if( ret == 0) return Handle(0);
	if( ret < 0) throw SocketException("Poll failed (receive)");

	ret = ::accept( m_socket, 0, 0);
	if( ret <= 0)
		throw SocketException("TCPSocket::timedAccept failed");
	return Handle(ret);
}
