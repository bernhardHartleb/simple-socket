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
	if( handle.m_sockfd == 0)
		throw SocketException("tried to initialize TCPSocket with invalid Handle");
}


int TCPSocket::sendAll( const void* buffer, int len)
{
	int sent = 0;
	while(sent != len)
	{
		int ret;
		if( (ret = ::send( m_socket, (raw_type*) (static_cast<const char*>(buffer) + sent), len - sent, 0)) < 0)
			throw SocketException("Send failed (send)");
		sent += ret;
		if(sent == len)
			break;
	}
	return sent;
}

void TCPSocket::listen( int backlog /* = 0 */) {
	::listen(m_socket, backlog);
}

TCPSocket::Handle TCPSocket::accept() const {
	int ret;
	if( (ret = ::accept( m_socket, 0, 0)) <= 0)
		throw SocketException("TCPSocket::accept failed");
	return Handle(ret);
}
