#include "UnixStreamSocket.h"
#include "TempFailure.h"

#include <sys/socket.h>
#include <poll.h>

using namespace NET;

UnixStreamSocket::UnixStreamSocket()
: UnixSocket( SOCK_STREAM, 0)
{}

UnixStreamSocket::UnixStreamSocket( Handle&& handle)
: UnixSocket( handle.release())
{}

void UnixStreamSocket::listen( int backlog)
{
	int ret = ::listen( m_socket, backlog);
	if( ret < 0)
		throw SocketException("UnixStreamSocket::listen failed");
}

UnixStreamSocket::Handle UnixStreamSocket::accept() const
{
#ifdef SOCK_CLOEXEC
	// prevent leaking socket descriptor on exec
	int ret = TEMP_FAILURE_RETRY (::accept4( m_socket, nullptr, nullptr, SOCK_CLOEXEC));
#else
	int ret = TEMP_FAILURE_RETRY (::accept( m_socket, nullptr, nullptr));
#endif
	if( ret < 0)
		throw SocketException("UnixStreamSocket::accept failed");

	return Handle(ret);
}

UnixStreamSocket::Handle UnixStreamSocket::timedAccept( int timeout) const
{
	struct pollfd poll;
	poll.fd = m_socket;
	poll.events = POLLIN;

	int ret = TEMP_FAILURE_RETRY (::poll( &poll, 1, timeout));
	if( ret == 0) return Handle();
	if( ret < 0)
		throw SocketException("UnixStreamSocket::timedAccept failed (poll)");

	if( poll.revents & (POLLERR | POLLNVAL))
		throw SocketException("UnixStreamSocket::timedAccept failed (poll)");

	return accept();
}
