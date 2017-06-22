#include "SocketUtils.h"
#include "SimpleSocket.h"

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netdb.h>
#include <unistd.h>
#include <cstdint>

#include <cstring>
#include <cstdlib>
#include <sstream>
#include <iomanip>

using namespace NET;

namespace
{
	// needed because of strict aliasing rules
	class sockaddr_ptr
	{
	public:
		explicit sockaddr_ptr( const sockaddr* sa) : msa(*sa) {}
		const sockaddr_in& operator*() const { return msi; }
		const sockaddr_in* operator->() const { return &msi; }

	private:
		union { sockaddr msa; sockaddr_in msi; };
	};

	// use as temporary RAII socket
	class temp_socket : public SimpleSocket
	{
	public:
		temp_socket() : SimpleSocket( AF_INET, SOCK_DGRAM, 0) {}
	};

	// check and copy the name of the interface
	void assign_ifreq( struct ifreq& ifr, const std::string& interface)
	{
		size_t len = interface.length();

		if( len >= sizeof(ifr.ifr_name))
			throw SocketException("Interface name is too long", false);

		ifr.ifr_addr.sa_family = AF_INET;
		std::memcpy( ifr.ifr_name, interface.data(), len);
		ifr.ifr_name[len] = 0;
	}
}

std::string NET::resolveHostname( const std::string& hostname)
{
	hostent* host = gethostbyname( hostname.c_str());
	if( host == nullptr)
	{
		// strerror() will not work for gethostbyname()
		throw SocketException("Failed to resolve address (gethostbyname)", false);
	}

	std::ostringstream ss;
	for(int i = 0; i < 4; i++) {
		if(i != 0)
			ss << '.';
		ss << static_cast<int>(host->h_addr[i]);
	}
	return ss.str();
}

uint16_t NET::resolveService( const std::string& service, const std::string& protocol)
{
	struct servent* serv;
	if(protocol == "")
		serv = getservbyname( service.c_str(), nullptr);
	else
		serv = getservbyname( service.c_str(), protocol.c_str());

	if(serv)
		return ntohs( static_cast<uint16_t>(serv->s_port));
	return 0;
}

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

std::string NET::getInterfaceAddress( const std::string& interface)
{
	struct ifreq ifr;
	assign_ifreq( ifr, interface);

	temp_socket sock;
	if( ioctl( sock.nativeHandle(), SIOCGIFADDR, &ifr) < 0)
		throw SocketException("ioctl failed (getInterfaceAddress)");

	return inet_ntoa( sockaddr_ptr( &ifr.ifr_addr)->sin_addr);
}

void NET::setInterfaceAddress( const std::string& interface, const std::string& address)
{
	struct ifreq ifr;
	assign_ifreq( ifr, interface);

	inet_aton( address.c_str(), &(reinterpret_cast<sockaddr_in*>(&ifr.ifr_addr)->sin_addr));

	temp_socket sock;
	if( ioctl( sock.nativeHandle(), SIOCSIFADDR, &ifr) < 0)
		throw SocketException("ioctl failed (setInterfaceAddress)");
}

std::string NET::getBroadcastAddress( const std::string& interface)
{
	struct ifreq ifr;
	assign_ifreq( ifr, interface);

	temp_socket sock;
	if( ioctl( sock.nativeHandle(), SIOCGIFBRDADDR, &ifr) < 0)
		throw SocketException("ioctl failed (getBroadcastAddress)");

	return inet_ntoa( sockaddr_ptr( &ifr.ifr_broadaddr)->sin_addr);
}

void NET::setBroadcastAddress( const std::string& interface, const std::string& address)
{
	struct ifreq ifr;
	assign_ifreq( ifr, interface);

	inet_aton( address.c_str(), &(reinterpret_cast<sockaddr_in*>(&ifr.ifr_broadaddr)->sin_addr));

	temp_socket sock;
	if( ioctl( sock.nativeHandle(), SIOCSIFBRDADDR, &ifr) < 0)
		throw SocketException("ioctl failed (setBroadcastAddress)");
}

std::string NET::getNetmask( const std::string& interface)
{
	struct ifreq ifr;
	assign_ifreq( ifr, interface);

	temp_socket sock;
	if( ioctl( sock.nativeHandle(), SIOCGIFNETMASK, &ifr) < 0)
		throw SocketException("ioctl failed (getNetmask)");

	return inet_ntoa( sockaddr_ptr( &ifr.ifr_netmask)->sin_addr);
}

void NET::setNetmask( const std::string& interface, const std::string& address)
{
	struct ifreq ifr;
	assign_ifreq( ifr, interface);

	inet_aton( address.c_str(), &(reinterpret_cast<sockaddr_in*>(&ifr.ifr_netmask)->sin_addr));

	temp_socket sock;
	if( ioctl( sock.nativeHandle(), SIOCSIFNETMASK, &ifr) < 0)
		throw SocketException("ioctl failed (setNetmask)");
}

std::string NET::getDestinationAddress( const std::string& interface)
{
	struct ifreq ifr;
	assign_ifreq( ifr, interface);

	temp_socket sock;
	if( ioctl( sock.nativeHandle(), SIOCGIFDSTADDR, &ifr) < 0)
		throw SocketException("ioctl failed (getDestinationAddress)");

	return inet_ntoa( sockaddr_ptr( &ifr.ifr_dstaddr)->sin_addr);
}

void NET::setDestinationAddress( const std::string& interface, const std::string& address)
{
	struct ifreq ifr;
	assign_ifreq( ifr, interface);

	inet_aton( address.c_str(), &(reinterpret_cast<sockaddr_in*>(&ifr.ifr_dstaddr)->sin_addr));

	temp_socket sock;
	if( ioctl( sock.nativeHandle(), SIOCSIFDSTADDR, &ifr) < 0)
		throw SocketException("ioctl failed (setDestinationAddress)");
}

int NET::getMTU( const std::string& interface)
{
	struct ifreq ifr;
	assign_ifreq( ifr, interface);

	temp_socket sock;
	if( ioctl( sock.nativeHandle(), SIOCGIFMTU, &ifr) < 0)
		throw SocketException("ioctl failed (getMTU)");

	return ifr.ifr_mtu;
}

void NET::setMTU( const std::string& interface, int mtu)
{
	struct ifreq ifr;
	assign_ifreq( ifr, interface);

	ifr.ifr_mtu = mtu;

	temp_socket sock;
	if( ioctl( sock.nativeHandle(), SIOCSIFMTU, &ifr) < 0)
		throw SocketException("ioctl failed (setMTU)");
}

std::string NET::getHardwareAddress( const std::string& interface, char separationChar)
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

	std::ostringstream str;
	str << std::hex << std::setw(2) << std::setfill('0');

	for( int i = 0; i <= 4; ++i)
	{
		str << std::uppercase
		    << static_cast<int>( reinterpret_cast<unsigned char*>( ifr.ifr_addr.sa_data)[i])
		    << separationChar;
	}

	str << std::uppercase
	    << static_cast<int>( reinterpret_cast<unsigned char*>( ifr.ifr_addr.sa_data)[5]);
	return str.str();
}
