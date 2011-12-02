#include "CANSocket.h"

#include <linux/can.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <cstring>

using namespace NET;

CANSocket::CANSocket( int type, int protocol)
: SimpleSocket( CAN, type, protocol)
{}

void CANSocket::connect( const std::string& interface)
{
	sockaddr_can addr;
	addr.can_family = AF_CAN;
	addr.can_ifindex = getInterfaceIndex(interface);

	if( ::connect( m_socket, (sockaddr*) &addr, sizeof(addr)) < 0)
		throw SocketException("Connect failed (connect)");
}

void CANSocket::bind( const std::string& interface)
{
	sockaddr_can addr;
	addr.can_family = AF_CAN;
	addr.can_ifindex = getInterfaceIndex(interface);

	if( ::bind( m_socket, (sockaddr*) &addr, sizeof(addr)) < 0)
		throw SocketException("Set of interface failed (bind)");
}

std::string CANSocket::getLocalInterface() const
{
	sockaddr_can addr;
	socklen_t addr_len = sizeof(addr);

	if( getsockname( m_socket, (sockaddr*) &addr, &addr_len) < 0)
		throw SocketException("Fetch of interface failed (getsockname)");

	return getInterfaceName(addr);
}

std::string CANSocket::getForeignInterface() const
{
	sockaddr_can addr;
	socklen_t addr_len = sizeof(addr);

	if( getpeername( m_socket, (sockaddr*) &addr, &addr_len) < 0)
		throw SocketException("Fetch of interface failed (getsockname)");

	return getInterfaceName(addr);
}

std::string CANSocket::getInterfaceName( const sockaddr_can& addr) const
{
	struct ifreq ifr;
	ifr.ifr_ifindex = addr.can_ifindex;

	if( ioctl( m_socket, SIOCGIFNAME, &ifr) < 0)
		throw SocketException("ioctl failed (getInterfaceName)");

	return std::string(ifr.ifr_name);
}

int CANSocket::getInterfaceIndex( const std::string& interface) const
{
	// binds to all interfaces
	if( interface.empty()) return 0;

	// needed space is size plus null character
	if( interface.size() >= IF_NAMESIZE)
		throw SocketException("Interface name is too long", false);

	struct ifreq ifr;
	std::strcpy( ifr.ifr_name, interface.c_str());

	if( ioctl( m_socket, SIOCGIFINDEX, &ifr) < 0)
		throw SocketException("ioctl failed (getInterfaceIndex)");

	return ifr.ifr_ifindex;
}
