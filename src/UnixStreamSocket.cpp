#include "UnixStreamSocket.h"
#include "TempFailure.h"

#include <sys/socket.h>

using namespace NET;

UnixStreamSocket::UnixStreamSocket()
: UnixSocket( SOCK_STREAM, 0)
{}

UnixStreamSocket UnixStreamSocket::accept() const
{
	int ret = SimpleSocket::accept();
	return UnixStreamSocket( AdoptSocketTag{}, ret);
}

std::optional<UnixStreamSocket> UnixStreamSocket::timedAccept( int timeout) const
{
	int ret = SimpleSocket::timedAccept(timeout);
	if( ret >= 0)
		return std::optional<UnixStreamSocket>{ UnixStreamSocket( AdoptSocketTag{}, ret)};
	else
		return std::nullopt;
}
