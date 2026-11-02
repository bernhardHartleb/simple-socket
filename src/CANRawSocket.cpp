#include "CANRawSocket.h"
#include "TempFailure.h"
#include "PollHelper.h"

#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>

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

	ssize_t ret = TEMP_FAILURE_RETRY (::sendto( m_socket, (const raw_type*)&frame, sizeof(frame), 0, (sockaddr*)&destAddr, sizeof(destAddr)));
	if( ret >= 0)
		return { IOStatus::Success, static_cast<size_t>(ret) };

	auto status = classifyIoError( errno, m_connectionOriented);
	if( status) return IOResult{ *status, 0 };

	throw SocketException("Send failed (sendto)");
}

IOResult CANRawSocket::receiveFrom( ::can_frame& frame, std::string& interface)
{
	sockaddr_can clientAddr;
	socklen_t addrLen = sizeof(clientAddr);

	ssize_t ret = TEMP_FAILURE_RETRY (::recvfrom( m_socket, (raw_type*)&frame, sizeof(frame), 0, (sockaddr*)&clientAddr, &addrLen));
	if( ret < 0)
	{
		auto status = classifyIoError( errno, m_connectionOriented);
		if( status) return IOResult{ *status, 0 };

		throw SocketException("Receive failed (recvfrom)");
	}
	interface = getInterfaceName(clientAddr);
	return { IOStatus::Success, static_cast<size_t>(ret) };
}

IOResult CANRawSocket::timedReceiveFrom( ::can_frame& frame, std::string& interface, int timeout)
{
	return timedReceiveImpl( m_socket, m_connectionOriented, timeout,
		[&] { return receiveFrom( frame, interface); });
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
