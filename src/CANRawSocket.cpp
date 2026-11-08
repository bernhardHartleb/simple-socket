#include "CANRawSocket.h"
#include "SockAddress.h"

#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <cstring>

using namespace NET;

CANRawSocket::CANRawSocket()
: CANSocket( SOCK_RAW, CAN_RAW)
{}

IOResult CANRawSocket::send( const ::can_frame& frame)
{
	return SimpleSocket::send( &frame, sizeof(frame));
}

IOResult CANRawSocket::receive( ::can_frame& frame)
{
	return SimpleSocket::receive( &frame, sizeof(frame));
}

IOResult CANRawSocket::timedReceive( ::can_frame& frame, int timeout)
{
	return SimpleSocket::timedReceive( &frame, sizeof(frame), timeout);
}

IOResult CANRawSocket::sendTo( const ::can_frame& frame, std::string_view interface)
{
	sockaddr_can destAddr;
	destAddr.can_family = AF_CAN;
	destAddr.can_ifindex = getInterfaceIndex(interface);

	SockAddress addr;
	std::memcpy( addr.data(), &destAddr, addr.length());

	return SimpleSocket::sendTo( (const void*)&frame, sizeof(frame), addr);
}

IOResult CANRawSocket::receiveFrom( ::can_frame& frame, std::string& interface)
{
	SockAddress addr;
	IOResult res = SimpleSocket::receiveFrom( (void*)&frame, sizeof(frame), addr);
	if( res.status == IOStatus::Success)
	{
		sockaddr_can clientAddr;
		std::memcpy( &clientAddr, addr.data(), sizeof(clientAddr));
		interface = getInterfaceName(clientAddr);
	}
	return res;
}

IOResult CANRawSocket::timedReceiveFrom( ::can_frame& frame, std::string& interface, int timeout)
{
	IOStatus res = SimpleSocket::timedReceive( timeout);
	if( res == IOStatus::Success)
		return receiveFrom( frame, interface);
	else
		return { res, 0 };
}

void CANRawSocket::setReceiveFilter( const ::can_filter* filter, size_t len)
{
	if( ::setsockopt( m_socket, SOL_CAN_RAW, CAN_RAW_FILTER, filter, sizeof(::can_filter) * len) < 0)
		throw SocketException("Set receive filter failed (setsockopt)");
}

void CANRawSocket::setErrorFilter( ::can_err_mask_t filter)
{
	if( ::setsockopt( m_socket, SOL_CAN_RAW, CAN_RAW_ERR_FILTER, &filter, sizeof(::can_err_mask_t)) < 0)
		throw SocketException("Set error filter failed (setsockopt)");
}

void CANRawSocket::setLocalLoopback( bool enable)
{
	const int value = enable;
	if( ::setsockopt( m_socket, SOL_CAN_RAW, CAN_RAW_LOOPBACK, &value, sizeof(value)) < 0)
		throw SocketException("Set local loopback failed (setsockopt)");
}

void CANRawSocket::setReceiveOwnMessages( bool enable)
{
	const int value = enable;
	if( ::setsockopt( m_socket, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS, &value, sizeof(value)) < 0)
		throw SocketException("Set receive own messages failed (setsockopt)");
}
