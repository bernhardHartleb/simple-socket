#include "TCPSocket.h"
#include "TempFailure.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

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

TCPSocket::Handle TCPSocket::accept() const
{
	int ret = SimpleSocket::accept();
	return Handle(ret);
}

TCPSocket::Handle TCPSocket::timedAccept( int timeout) const
{
	int ret = SimpleSocket::timedAccept(timeout);
	return Handle(ret);
}
