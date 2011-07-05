#include "UnixDatagramSocket.h"
#include "TempFailure.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>

using namespace NET;

UnixDatagramSocket::UnixDatagramSocket()
: UnixSocket( DATAGRAM, 0)
{}

void UnixDatagramSocket::sendTo( const void* buffer, size_t len, const std::string& foreignPath)
{
	sockaddr_un destAddr;
	fillAddress( foreignPath, destAddr);

	int sent = TEMP_FAILURE_RETRY (::sendto( m_socket, (const raw_type*)buffer, len, 0, (sockaddr*)&destAddr, sizeof(destAddr)));

	// Write out the whole buffer as a single message
	if( sent != (int)len)
		throw SocketException("Send failed (sendto)");
}

int UnixDatagramSocket::receiveFrom( void* buffer, size_t len, std::string& sourcePath)
{
	sockaddr_un clientAddr;
	socklen_t addr_len = sizeof(clientAddr);

	int ret = TEMP_FAILURE_RETRY (::recvfrom( m_socket, (raw_type*)buffer, len, 0, (sockaddr*)&clientAddr, &addr_len));
	if( ret < 0)
		throw SocketException("Receive failed (recvfrom)");

	sourcePath = extractPath( clientAddr, addr_len);
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

	if( poll.revents & POLLRDHUP)
		m_peerDisconnected = true;

	if( poll.revents & POLLIN || poll.revents & POLLPRI)
		return receiveFrom( buffer, len, sourcePath);

	return 0;
}
