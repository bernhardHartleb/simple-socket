#include "TCPSocket.h"
#include "TempFailure.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

using namespace NET;

TCPSocket::TCPSocket( AddressFamily family)
: InternetSocket( family, SOCK_STREAM, IPPROTO_TCP)
{}

void TCPSocket::setNoDelay( bool enable)
{
	const int value = enable;
	if( ::setsockopt( m_socket, IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value)) < 0)
		throw SocketException("Failed to set TCP_NODELAY (setsockopt)");
}

TCPSocket TCPSocket::accept() const
{
	int ret = SimpleSocket::accept();
	return TCPSocket(ret);
}

std::optional<NET::TCPSocket> TCPSocket::timedAccept( int timeout) const
{
	int ret = SimpleSocket::timedAccept(timeout);
	if( ret > 0)
		return std::optional<NET::TCPSocket>{ TCPSocket(ret)};
	else
		return std::nullopt;
}
