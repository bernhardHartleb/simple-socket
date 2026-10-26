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
	int fd = TEMP_FAILURE_RETRY (::accept( m_socket, nullptr, nullptr));
	if( fd < 0)
		throw SocketException("UnixStreamSocket::accept failed");

	return Handle(fd);
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
