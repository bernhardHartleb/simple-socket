#include "UnixStreamSocket.h"
#include "TempFailure.h"

#include <sys/socket.h>

using namespace NET;

UnixStreamSocket::UnixStreamSocket()
: UnixSocket( SOCK_STREAM, 0)
{}

UnixStreamSocket::UnixStreamSocket( Handle&& handle)
: UnixSocket( handle.release())
{}

UnixStreamSocket::Handle UnixStreamSocket::accept() const
{
	int ret = SimpleSocket::accept();
	return Handle(ret);
}

UnixStreamSocket::Handle UnixStreamSocket::timedAccept( int timeout) const
{
	int ret = SimpleSocket::timedAccept(timeout);
	return Handle(ret);
}
