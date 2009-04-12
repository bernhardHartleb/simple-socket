#include "TCPSocket.h"

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
	::listen(m_socket, backlog);
}

TCPSocket::Handle TCPSocket::accept() const
{
	int ret;
	if( (ret = ::accept( m_socket, 0, 0)) <= 0)
		throw SocketException("TCPSocket::accept failed");
	return Handle(ret);
}
