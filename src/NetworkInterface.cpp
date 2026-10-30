#include "NetworkInterface.h"
#include "SockAddress.h"

#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <net/if_arp.h>

#include <cstring>
#include <memory>
#include <utility>

using namespace NET;

namespace
{
	// use as temporary RAII socket
	class temp_socket : public SimpleSocket
	{
	public:
		temp_socket() : SimpleSocket( AF_INET, SOCK_DGRAM, 0) {}
	};

	in_addr ipv4Address( const sockaddr& sa)
	{
		sockaddr_in addr = {};
		std::memcpy( &addr, &sa, sizeof(addr));
		return addr.sin_addr;
	}

	void assign_ipv4( sockaddr& sa, std::string_view address)
	{
		static_assert( sizeof(sockaddr_in) <= sizeof(sockaddr),
		               "sockaddr must be large enough for an IPv4 address");

		std::string address_z(address);
		sockaddr_in addr = {};
		addr.sin_family = AF_INET;

		if( inet_aton( address_z.c_str(), &addr.sin_addr) == 0)
			throw SocketException("Invalid IPv4 address", false);

		std::memcpy( &sa, &addr, sizeof(addr));
	}

	// check and copy the name of the interface
	void assign_ifreq( struct ifreq& ifr, std::string_view interface)
	{
		const size_t len = interface.length();

		if( len >= IF_NAMESIZE)
			throw SocketException("Interface name is too long", false);

		ifr = {};
		ifr.ifr_addr.sa_family = AF_INET;
		std::memcpy( ifr.ifr_name, interface.data(), len);
		ifr.ifr_name[len] = 0;
	}

	std::string addressString( const sockaddr* address)
	{
		return SockAddress(address).address();
	}
}

std::vector<NetworkAddressInfo> NET::getNetworkAddresses()
{
	ifaddrs* interfaces = nullptr;
	if( ::getifaddrs(&interfaces) < 0)
		throw SocketException("getifaddrs failed (getNetworkInterfaces)");

	std::unique_ptr<ifaddrs, decltype(&::freeifaddrs)>
		interfaces_guard(interfaces, &::freeifaddrs);

	std::vector<NetworkAddressInfo> ret;
	for( const ifaddrs* entry = interfaces; entry != nullptr; entry = entry->ifa_next)
	{
		if( entry->ifa_name == nullptr || entry->ifa_addr == nullptr)
			continue;

		const sa_family_t family = entry->ifa_addr->sa_family;
		if( family != AF_INET && family != AF_INET6)
			continue;

		std::string broadcast;
		std::string destination;

		// ifa_broadaddr and ifa_dstaddr share storage on common
		// getifaddrs implementations, so the flags determine which is valid.
		if( entry->ifa_broadaddr && (entry->ifa_flags & IFF_BROADCAST) != 0)
			broadcast = addressString(entry->ifa_broadaddr);

		if( entry->ifa_dstaddr && (entry->ifa_flags & IFF_POINTOPOINT) != 0)
			destination = addressString(entry->ifa_dstaddr);

		ret.emplace_back(
			std::string(entry->ifa_name),
			addressString(entry->ifa_addr),
			addressString(entry->ifa_netmask),
			std::move(broadcast),
			std::move(destination),
			(family == AF_INET ? AddressFamily::IPv4 : AddressFamily::IPv6),
			entry->ifa_flags
		);
	}
	return ret;
}

std::vector<NetworkInterface> NET::getNetworkInterfaces()
{
	std::vector<NetworkInterface> ret;
	struct if_nameindex* index = ::if_nameindex();
	if(index == nullptr) return ret;

	for( int i = 0; (index[i].if_index != 0); ++i)
		ret.push_back( NetworkInterface( std::string( index[i].if_name)));

	if_freenameindex(index);
	return ret;
}


NetworkInterface::NetworkInterface( std::string_view interface)
: interface(interface)
{}

std::string NetworkInterface::getInterfaceAddress() const
{
	struct ifreq ifr;
	assign_ifreq( ifr, interface);

	temp_socket sock;
	if( ioctl( sock.nativeHandle(), SIOCGIFADDR, &ifr) < 0)
		return std::string();

	return inet_ntoa( ipv4Address(ifr.ifr_addr));
}

void NetworkInterface::setInterfaceAddress( std::string_view address)
{
	struct ifreq ifr;
	assign_ifreq( ifr, interface);
	assign_ipv4( ifr.ifr_addr, address);

	temp_socket sock;
	if( ioctl( sock.nativeHandle(), SIOCSIFADDR, &ifr) < 0)
		throw SocketException("ioctl failed (setInterfaceAddress)");
}

std::string NetworkInterface::getBroadcastAddress() const
{
	struct ifreq ifr;
	assign_ifreq( ifr, interface);

	temp_socket sock;
	if( ioctl( sock.nativeHandle(), SIOCGIFBRDADDR, &ifr) < 0)
		return std::string();

	return inet_ntoa( ipv4Address(ifr.ifr_broadaddr));
}

void NetworkInterface::setBroadcastAddress( std::string_view address)
{
	struct ifreq ifr;
	assign_ifreq( ifr, interface);
	assign_ipv4( ifr.ifr_broadaddr, address);

	temp_socket sock;
	if( ioctl( sock.nativeHandle(), SIOCSIFBRDADDR, &ifr) < 0)
		throw SocketException("ioctl failed (setBroadcastAddress)");
}

std::string NetworkInterface::getNetmask() const
{
	struct ifreq ifr;
	assign_ifreq( ifr, interface);

	temp_socket sock;
	if( ioctl( sock.nativeHandle(), SIOCGIFNETMASK, &ifr) < 0)
		return std::string();

	return inet_ntoa( ipv4Address(ifr.ifr_netmask));
}

void NetworkInterface::setNetmask( std::string_view address)
{
	struct ifreq ifr;
	assign_ifreq( ifr, interface);
	assign_ipv4( ifr.ifr_netmask, address);

	temp_socket sock;
	if( ioctl( sock.nativeHandle(), SIOCSIFNETMASK, &ifr) < 0)
		throw SocketException("ioctl failed (setNetmask)");
}

std::string NetworkInterface::getDestinationAddress() const
{
	struct ifreq ifr;
	assign_ifreq( ifr, interface);

	temp_socket sock;
	if( ioctl( sock.nativeHandle(), SIOCGIFDSTADDR, &ifr) < 0)
		return std::string();

	return inet_ntoa( ipv4Address(ifr.ifr_dstaddr));
}

void NetworkInterface::setDestinationAddress( std::string_view address)
{
	struct ifreq ifr;
	assign_ifreq( ifr, interface);
	assign_ipv4( ifr.ifr_dstaddr, address);

	temp_socket sock;
	if( ioctl( sock.nativeHandle(), SIOCSIFDSTADDR, &ifr) < 0)
		throw SocketException("ioctl failed (setDestinationAddress)");
}

int NetworkInterface::getMTU() const
{
	struct ifreq ifr;
	assign_ifreq( ifr, interface);

	temp_socket sock;
	if( ioctl( sock.nativeHandle(), SIOCGIFMTU, &ifr) < 0)
		return 0;

	return ifr.ifr_mtu;
}

void NetworkInterface::setMTU( int mtu)
{
	struct ifreq ifr;
	assign_ifreq( ifr, interface);

	ifr.ifr_mtu = mtu;

	temp_socket sock;
	if( ioctl( sock.nativeHandle(), SIOCSIFMTU, &ifr) < 0)
		throw SocketException("ioctl failed (setMTU)");
}

std::string NetworkInterface::getHardwareAddress() const
{
	struct ifreq ifr;
	assign_ifreq( ifr, interface);

	temp_socket sock;
	if( ioctl( sock.nativeHandle(), SIOCGIFHWADDR, &ifr) < 0)
		return std::string();

	switch( ifr.ifr_hwaddr.sa_family) {
		default:
			return std::string();
			break;
		case ARPHRD_NETROM:
		case ARPHRD_ETHER:
		case ARPHRD_PPP:
		case ARPHRD_EETHER:
		case ARPHRD_IEEE802:
			break;
	}

	char mac[20];
	auto addr = reinterpret_cast<unsigned char*>( ifr.ifr_addr.sa_data);
	int len = sprintf(mac, "%02x:%02x:%02x:%02x:%02x:%02x", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
	return std::string(mac, static_cast<size_t>(len));
}
