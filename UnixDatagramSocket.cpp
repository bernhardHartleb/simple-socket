#include "UnixDatagramSocket.h"
#include "SockAddress.h"
#include "TempFailure.h"

#include <sys/socket.h>
#include <poll.h>

using namespace NET;

UnixDatagramSocket::UnixDatagramSocket()
: UnixSocket( SOCK_DGRAM, 0)
{}

void UnixDatagramSocket::sendTo( const void* buffer, size_t len, std::string_view foreignPath)
{
	SockAddress addr( foreignPath);

	int sent = TEMP_FAILURE_RETRY (::sendto( m_socket, (const raw_type*)buffer, len, 0, addr.data(), addr.length()));

	// Write out the whole buffer as a single message
	if( sent != (int)len)
		throw SocketException("Send failed (sendto)");
}

int UnixDatagramSocket::receiveFrom( void* buffer, size_t len, std::string& sourcePath)
{
	SockAddress addr;

	int ret = TEMP_FAILURE_RETRY (::recvfrom( m_socket, (raw_type*)buffer, len, 0, addr.data(), addr.lengthPtr()));
	if( ret < 0)
		throw SocketException("Receive failed (recvfrom)");

	sourcePath = addr.address();
	return ret;
}

int UnixDatagramSocket::timedReceiveFrom( void* buffer, size_t len, std::string& sourcePath, int timeout)
{
	struct pollfd poll;
	poll.fd = m_socket;
	poll.events = POLLIN | POLLPRI | POLLRDHUP;

	int ret = TEMP_FAILURE_RETRY (::poll( &poll, 1, timeout));

	if( ret == 0) return 0;
	if( ret < 0)  throw SocketException("Receive failed (poll)");

	if( poll.revents & POLLRDHUP || poll.revents & POLLHUP)
		m_peerDisconnected = true;

	if( poll.revents & POLLIN || poll.revents & POLLPRI)
		return receiveFrom( buffer, len, sourcePath);

	if( poll.revents & POLLERR)
		throw SocketException("timedReceive failed (poll)");

	return 0;
}
