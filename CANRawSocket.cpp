#include "CANRawSocket.h"
#include "TempFailure.h"

#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <poll.h>

using namespace NET;

CANRawSocket::CANRawSocket()
: CANSocket( RAW, CAN_RAW)
{}

void CANRawSocket::sendTo( const void* buffer, size_t len, const std::string& interface)
{
	sockaddr_can destAddr;
	destAddr.can_family = AF_CAN;
	destAddr.can_ifindex = getInterfaceIndex(interface);

	int sent = TEMP_FAILURE_RETRY (::sendto( m_socket, (const raw_type*)buffer, len, 0, (sockaddr*)&destAddr, sizeof(destAddr)));

	// Write out the whole buffer as a single message
	if( sent != (int)len)
		throw SocketException("Send failed (sendto)");
}

int CANRawSocket::receiveFrom( void* buffer, size_t len, std::string& interface)
{
	sockaddr_can clientAddr;
	socklen_t addrLen = sizeof(clientAddr);

	int ret = TEMP_FAILURE_RETRY (::recvfrom( m_socket, (raw_type*)buffer, len, 0, (sockaddr*)&clientAddr, &addrLen));
	if( ret < 0)
		throw SocketException("Receive failed (recvfrom)");

	interface = getInterfaceName(clientAddr);
	return ret;
}

int CANRawSocket::timedReceiveFrom( void* buffer, size_t len, std::string& interface, int timeout)
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
		return receiveFrom( buffer, len, interface);

	return 0;
}

void CANRawSocket::setReceiveFilter( const can_filter* filter, size_t len)
{
	if( setsockopt( m_socket,
	                SOL_CAN_RAW,
	                CAN_RAW_FILTER,
	                (const raw_type*)filter,
	                sizeof(filter) * len) < 0)
		throw SocketException("Set receive filter failed (setsockopt)");
}

void CANRawSocket::setErrorFilter( const can_filter* filter, size_t len)
{
	if( setsockopt( m_socket,
	                SOL_CAN_RAW,
	                CAN_RAW_ERR_FILTER,
	                (const raw_type*)filter,
	                sizeof(filter) * len) < 0)
		throw SocketException("Set receive filter failed (setsockopt)");
}

void CANRawSocket::setLocalLoopback( bool enable)
{
	int loopback = enable;
	if( setsockopt( m_socket,
	                SOL_CAN_RAW,
	                CAN_RAW_LOOPBACK,
	                (raw_type*)&loopback,
	                sizeof(loopback)) < 0)
		throw SocketException("Set local loopback failed (setsockopt)");
}

void CANRawSocket::setReceiveOwnMessages( bool enable)
{
	int recv_own_msgs = enable;
	if( setsockopt( m_socket,
	                SOL_CAN_RAW,
	                CAN_RAW_RECV_OWN_MSGS,
	                (raw_type*)&recv_own_msgs,
	                sizeof(recv_own_msgs)) < 0)
		throw SocketException("Set receive own messages failed (setsockopt)");
}
