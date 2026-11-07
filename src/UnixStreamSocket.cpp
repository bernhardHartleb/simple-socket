#include "UnixStreamSocket.h"

#include <sys/socket.h>

using namespace NET;

UnixStreamSocket::UnixStreamSocket()
: UnixSocket( SOCK_STREAM, 0)
{}

UnixStreamSocket UnixStreamSocket::adoptNativeHandle( int sockfd)
{
	validateSocket( sockfd, SOCK_STREAM, 0);
	return UnixStreamSocket( AdoptSocketTag{}, sockfd, SOCK_STREAM);
}

std::optional<UnixStreamSocket> UnixStreamSocket::accept() const
{
	int ret = SimpleSocket::accept();
	if( ret >= 0)
		return std::optional<UnixStreamSocket>{ UnixStreamSocket( AdoptSocketTag{}, ret, SOCK_STREAM)};
	else
		return std::nullopt;
}

std::optional<UnixStreamSocket> UnixStreamSocket::timedAccept( int timeout) const
{
	int ret = SimpleSocket::timedAccept(timeout);
	if( ret >= 0)
		return std::optional<UnixStreamSocket>{ UnixStreamSocket( AdoptSocketTag{}, ret, SOCK_STREAM)};
	else
		return std::nullopt;
}
