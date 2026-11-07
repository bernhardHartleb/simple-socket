#include "TCPSocket.h"
#include "TempFailure.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

using namespace NET;

TCPSocket::TCPSocket( AddressFamily family)
: InternetSocket( family, SOCK_STREAM, IPPROTO_TCP)
{}

TCPSocket TCPSocket::adoptNativeHandle( int sockfd)
{
	validateSocket( sockfd, SOCK_STREAM, IPPROTO_TCP);
	return TCPSocket( AdoptSocketTag{}, sockfd, SOCK_STREAM);
}

void TCPSocket::setNoDelay( bool enable)
{
	const int value = enable;
	if( ::setsockopt( m_socket, IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value)) < 0)
		throw SocketException("Failed to set TCP_NODELAY (setsockopt)");
}

std::optional<TCPSocket> TCPSocket::accept() const
{
	int ret = SimpleSocket::accept();
	if( ret >= 0)
		return std::optional<TCPSocket>{ TCPSocket( AdoptSocketTag{}, ret, SOCK_STREAM)};
	else
		return std::nullopt;
}

std::optional<TCPSocket> TCPSocket::timedAccept( int timeout) const
{
	int ret = SimpleSocket::timedAccept(timeout);
	if( ret >= 0)
		return std::optional<TCPSocket>{ TCPSocket( AdoptSocketTag{}, ret, SOCK_STREAM)};
	else
		return std::nullopt;
}
