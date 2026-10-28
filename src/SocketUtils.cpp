#include "SocketUtils.h"
#include "SimpleSocket.h"

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netdb.h>
#include <unistd.h>

#include <cstring>
#include <memory>

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
}

std::string NET::resolveHostname( const std::string& hostname)
{
	addrinfo hints = {};
	hints.ai_family = AF_INET;

	addrinfo* result = nullptr;
	int status = getaddrinfo( hostname.c_str(), nullptr, &hints, &result);
	if( status != 0 || result == nullptr)
		throw SocketException("Failed to resolve address (getaddrinfo)", false);

	std::unique_ptr<addrinfo, decltype(&::freeaddrinfo)>
		result_guard(result, &::freeaddrinfo);

	if( result->ai_addr == nullptr || result->ai_addrlen < sizeof(sockaddr_in))
		throw SocketException("Resolver returned an invalid IPv4 address", false);

	sockaddr_in addr = {};
	std::memcpy( &addr, result->ai_addr, sizeof(addr));
	return inet_ntoa( addr.sin_addr);
}

uint16_t NET::resolveService( const std::string& service, const std::string& protocol)
{
	struct servent* serv;
	if( protocol.empty())
		serv = getservbyname( service.c_str(), nullptr);
	else
		serv = getservbyname( service.c_str(), protocol.c_str());

	if(serv)
		return ntohs( static_cast<uint16_t>(serv->s_port));
	return 0;
}

/* This function does not understand interfaces with multiple addresses
 * For example when using zeroconf (eth0, eth0:avahi) it would only list eth0
 * In this case eth0 does not have an ip address yet and getInterfaceAddress("eth0") throws
std::vector<std::string> NET::getNetworkInterfaces()
{
	std::vector<std::string> ret;
	struct if_nameindex* index = if_nameindex();
	if(index == nullptr) return ret;

	for( int i = 0; (index[i].if_index != 0); ++i)
		ret.push_back( std::string( index[i].if_name));

	if_freenameindex(index);
	return ret;
}
*/

std::vector<std::string> NET::getNetworkInterfaces()
{
	std::vector<std::string> ret;
	// maximum of number interfaces
	char data[32 * sizeof(struct ifreq)];
	struct ifconf conf;
	struct ifreq *ifr;

	conf.ifc_len = sizeof(data);
	conf.ifc_buf = (caddr_t) data;

	temp_socket sock;
	if( ioctl( sock.nativeHandle(), SIOCGIFCONF, &conf) < 0)
		throw SocketException("ioctl failed (getNetworkInterfaces)");

	ifr = (struct ifreq*)data;
	while( (char*)ifr < data+conf.ifc_len)
	{
		switch(ifr->ifr_addr.sa_family)
		{
		case AF_INET:
			ret.emplace_back( ifr->ifr_name);
			//printf("%s : %s\n", ifr->ifr_name, inet_ntop(ifr->ifr_addr.sa_family, &((struct sockaddr_in*)&ifr->ifr_addr)->sin_addr, addrbuf, sizeof(addrbuf)));
			break;
	#if 0
		case AF_INET6:
			printf("%s : %s\n", ifr->ifr_name, inet_ntop(ifr->ifr_addr.sa_family, &((struct sockaddr_in6*)&ifr->ifr_addr)->sin6_addr, addrbuf, sizeof(addrbuf)));
			break;
	#endif
		default:
			break;
		}
		++ifr;
	}
	return ret;
}

std::string NET::getInterfaceAddress( std::string_view interface)
{
	struct ifreq ifr;
	assign_ifreq( ifr, interface);

	temp_socket sock;
	if( ioctl( sock.nativeHandle(), SIOCGIFADDR, &ifr) < 0)
		throw SocketException("ioctl failed (getInterfaceAddress)");

	return inet_ntoa( ipv4Address(ifr.ifr_addr));
}

void NET::setInterfaceAddress( std::string_view interface, std::string_view address)
{
	struct ifreq ifr;
	assign_ifreq( ifr, interface);
	assign_ipv4( ifr.ifr_addr, address);

	temp_socket sock;
	if( ioctl( sock.nativeHandle(), SIOCSIFADDR, &ifr) < 0)
		throw SocketException("ioctl failed (setInterfaceAddress)");
}

std::string NET::getBroadcastAddress( std::string_view interface)
{
	struct ifreq ifr;
	assign_ifreq( ifr, interface);

	temp_socket sock;
	if( ioctl( sock.nativeHandle(), SIOCGIFBRDADDR, &ifr) < 0)
		throw SocketException("ioctl failed (getBroadcastAddress)");

	return inet_ntoa( ipv4Address(ifr.ifr_broadaddr));
}

void NET::setBroadcastAddress( std::string_view interface, std::string_view address)
{
	struct ifreq ifr;
	assign_ifreq( ifr, interface);
	assign_ipv4( ifr.ifr_broadaddr, address);

	temp_socket sock;
	if( ioctl( sock.nativeHandle(), SIOCSIFBRDADDR, &ifr) < 0)
		throw SocketException("ioctl failed (setBroadcastAddress)");
}

std::string NET::getNetmask( std::string_view interface)
{
	struct ifreq ifr;
	assign_ifreq( ifr, interface);

	temp_socket sock;
	if( ioctl( sock.nativeHandle(), SIOCGIFNETMASK, &ifr) < 0)
		throw SocketException("ioctl failed (getNetmask)");

	return inet_ntoa( ipv4Address(ifr.ifr_netmask));
}

void NET::setNetmask( std::string_view interface, std::string_view address)
{
	struct ifreq ifr;
	assign_ifreq( ifr, interface);
	assign_ipv4( ifr.ifr_netmask, address);

	temp_socket sock;
	if( ioctl( sock.nativeHandle(), SIOCSIFNETMASK, &ifr) < 0)
		throw SocketException("ioctl failed (setNetmask)");
}

std::string NET::getDestinationAddress( std::string_view interface)
{
	struct ifreq ifr;
	assign_ifreq( ifr, interface);

	temp_socket sock;
	if( ioctl( sock.nativeHandle(), SIOCGIFDSTADDR, &ifr) < 0)
		throw SocketException("ioctl failed (getDestinationAddress)");

	return inet_ntoa( ipv4Address(ifr.ifr_dstaddr));
}

void NET::setDestinationAddress( std::string_view interface, std::string_view address)
{
	struct ifreq ifr;
	assign_ifreq( ifr, interface);
	assign_ipv4( ifr.ifr_dstaddr, address);

	temp_socket sock;
	if( ioctl( sock.nativeHandle(), SIOCSIFDSTADDR, &ifr) < 0)
		throw SocketException("ioctl failed (setDestinationAddress)");
}

int NET::getMTU( std::string_view interface)
{
	struct ifreq ifr;
	assign_ifreq( ifr, interface);

	temp_socket sock;
	if( ioctl( sock.nativeHandle(), SIOCGIFMTU, &ifr) < 0)
		throw SocketException("ioctl failed (getMTU)");

	return ifr.ifr_mtu;
}

void NET::setMTU( std::string_view interface, int mtu)
{
	struct ifreq ifr;
	assign_ifreq( ifr, interface);

	ifr.ifr_mtu = mtu;

	temp_socket sock;
	if( ioctl( sock.nativeHandle(), SIOCSIFMTU, &ifr) < 0)
		throw SocketException("ioctl failed (setMTU)");
}

std::string NET::getHardwareAddress( std::string_view interface)
{
	struct ifreq ifr;
	assign_ifreq( ifr, interface);

	temp_socket sock;
	if( ioctl( sock.nativeHandle(), SIOCGIFHWADDR, &ifr) < 0)
		throw SocketException("ioctl failed (getHardwareAddress)");

	switch( ifr.ifr_hwaddr.sa_family) {
		default:
			throw SocketException("Invalid Hardware type (getHardwareAddress)");
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
