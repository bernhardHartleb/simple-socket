#include "UnixStreamSocket.h"
#include "TempFailure.h"

#include <poll.h>
#include <sys/socket.h>

using namespace NET;

UnixStreamSocket::UnixStreamSocket()
: UnixSocket( STREAM, 0)
{}

UnixStreamSocket::UnixStreamSocket( Handle&& handle)
: UnixSocket( handle.release())
{}

int UnixStreamSocket::sendAll( const void* buffer, size_t len)
{
	size_t sent = 0;
	while( sent != len)
	{
		const char* buf = static_cast<const char*>(buffer) + sent;
		auto ret = send( buf, len - sent);
		sent += static_cast<size_t>(ret);
		if( m_peerDisconnected) break;
	}
	return sent;
}

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
	if( ret == 0)
		return Handle();
	if( ret < 0)
		throw SocketException("UnixStreamSocket::timedAccept failed (poll)");

	if( poll.revents & (POLLERR | POLLNVAL))
		throw SocketException("UnixStreamSocket::timedAccept failed (poll)");

	return accept();
}
