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
#include <cstdlib>
#include <sstream>
#include <iomanip>

using namespace NET;

namespace
{
	class sockaddr_ptr
	{
	public:
		sockaddr_ptr( const sockaddr* sa) : msa(*sa) {}
		const sockaddr_in& operator*() const { return msi; }
		const sockaddr_in* operator->() const { return &msi; }

	private:
		union { sockaddr msa; sockaddr_in msi; };
	};
}

std::string NET::resolveHostname( const std::string& hostname)
{
	hostent* host = gethostbyname( hostname.c_str());
	if( host == 0)
	{
		// strerror() will not work for gethostbyname()
		throw SocketException("Failed to resolve address (gethostbyname)", false);
	}
	return std::string(host->h_addr);
}

unsigned short NET::resolveService( const std::string& service, const std::string& protocol)
{
	struct servent* serv = getservbyname( service.c_str(), protocol.c_str());

	if(!serv)
		// Service is port number
		return static_cast<unsigned short>(std::atoi( service.c_str()));
	else
		// Found port (network byte order) by name
		return static_cast<unsigned short>(ntohs( serv->s_port));
}

std::vector<std::string> NET::getNetworkInterfaces()
{
	std::vector<std::string> ret;
	struct if_nameindex* index = if_nameindex();

	for( int i = 0; index[i].if_index != 0; ++i)
		ret.push_back( std::string( index[i].if_name));

	if_freenameindex(index);
	return ret;
}

std::string NET::getInterfaceAddress( const std::string& interface)
{
	struct ifreq ifr;

	ifr.ifr_addr.sa_family = AF_INET;
	std::strncpy( ifr.ifr_name, interface.c_str(), sizeof(ifr.ifr_name));

	int sock = ::socket( AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if( sock < 0)
		throw SocketException("Couldn't create socket (getInterfaceAddress)");

	if( ioctl( sock, SIOCGIFADDR, &ifr) < 0)
		throw SocketException("ioctl failed (getInterfaceAddress)");

	::close(sock);

	return inet_ntoa( sockaddr_ptr( &ifr.ifr_addr)->sin_addr);
}

void NET::setInterfaceAddress( const std::string& interface, const std::string& address)
{
	struct ifreq ifr;

	ifr.ifr_addr.sa_family = AF_INET;
	std::strncpy( ifr.ifr_name, interface.c_str(), sizeof(ifr.ifr_name));

	int sock = ::socket( AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if( sock < 0)
		throw SocketException("Couldn't create socket (setInterfaceAddress)");

	inet_aton( address.c_str(), &(reinterpret_cast<sockaddr_in*>(&ifr.ifr_addr)->sin_addr));

	if( ioctl( sock, SIOCSIFADDR, &ifr) < 0)
		throw SocketException("ioctl failed (setInterfaceAddress)");

	::close(sock);
}

std::string NET::getBroadcastAddress( const std::string& interface)
{
	struct ifreq ifr;

	ifr.ifr_addr.sa_family = AF_INET;
	std::strncpy( ifr.ifr_name, interface.c_str(), sizeof(ifr.ifr_name));

	int sock = ::socket( AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if( sock < 0)
		throw SocketException("Couldn't create socket (getBroadcastAddress)");

	if( ioctl( sock, SIOCGIFBRDADDR, &ifr) < 0)
		throw SocketException("ioctl failed (getInterfaceAddress)");

	::close(sock);

	return inet_ntoa( sockaddr_ptr( &ifr.ifr_broadaddr)->sin_addr);
}

void NET::setBroadcastAddress( const std::string& interface, const std::string& address)
{
	struct ifreq ifr;

	ifr.ifr_addr.sa_family = AF_INET;
	std::strncpy( ifr.ifr_name, interface.c_str(), sizeof(ifr.ifr_name));

	int sock = ::socket( AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if( sock < 0)
		throw SocketException("Couldn't create socket (setBroadcastAddress)");

	inet_aton( address.c_str(), &(reinterpret_cast<sockaddr_in*>(&ifr.ifr_broadaddr)->sin_addr));

	if( ioctl( sock, SIOCSIFBRDADDR, &ifr) < 0)
		throw SocketException("ioctl failed (setBroadcastAddress)");

	::close(sock);
}

std::string NET::getNetmask( const std::string& interface)
{
	struct ifreq ifr;

	ifr.ifr_addr.sa_family = AF_INET;
	std::strncpy( ifr.ifr_name, interface.c_str(), sizeof(ifr.ifr_name));

	int sock = ::socket( AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if( sock < 0)
		throw SocketException("Couldn't create socket (getBroadcastAddress)");

	if( ioctl( sock, SIOCGIFNETMASK, &ifr) < 0)
		throw SocketException("ioctl failed (getNetmask)");

	::close(sock);

	return inet_ntoa( sockaddr_ptr( &ifr.ifr_netmask)->sin_addr);
}

void NET::setNetmask( const std::string& interface, const std::string& address)
{
	struct ifreq ifr;

	ifr.ifr_addr.sa_family = AF_INET;
	std::strncpy( ifr.ifr_name, interface.c_str(), sizeof(ifr.ifr_name));

	int sock = ::socket( AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if( sock < 0)
		throw SocketException("Couldn't create socket (setNetmask)");

	inet_aton( address.c_str(), &(reinterpret_cast<sockaddr_in*>(&ifr.ifr_netmask)->sin_addr));

	if( ioctl( sock, SIOCSIFNETMASK, &ifr) < 0)
		throw SocketException("ioctl failed (setNetmask)");

	::close(sock);
}

std::string NET::getDestinationAddress( const std::string& interface)
{
	struct ifreq ifr;

	ifr.ifr_addr.sa_family = AF_INET;
	std::strncpy( ifr.ifr_name, interface.c_str(), sizeof(ifr.ifr_name));

	int sock = ::socket( AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if( sock < 0)
		throw SocketException("Couldn't create socket (getBroadcastAddress)");

	if( ioctl( sock, SIOCGIFDSTADDR, &ifr) < 0)
		throw SocketException("ioctl failed (getDestinationAddress)");

	::close(sock);

	return inet_ntoa( sockaddr_ptr( &ifr.ifr_dstaddr)->sin_addr);
}

void NET::setDestinationAddress( const std::string& interface, const std::string& address)
{
	struct ifreq ifr;

	ifr.ifr_addr.sa_family = AF_INET;
	std::strncpy( ifr.ifr_name, interface.c_str(), sizeof(ifr.ifr_name));

	int sock = ::socket( AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if( sock < 0)
		throw SocketException("Couldn't create socket (setDestinationAddress)");

	inet_aton( address.c_str(), &(reinterpret_cast<sockaddr_in*>(&ifr.ifr_dstaddr)->sin_addr));

	if( ioctl( sock, SIOCSIFDSTADDR, &ifr) < 0)
		throw SocketException("ioctl failed (setDestinationAddress)");

	::close(sock);
}

int NET::getMTU( const std::string& interface)
{
	struct ifreq ifr;

	ifr.ifr_addr.sa_family = AF_INET;
	std::strncpy( ifr.ifr_name, interface.c_str(), sizeof(ifr.ifr_name));

	int sock = ::socket( AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if( sock < 0)
		throw SocketException("Couldn't create socket (getMTU)");

	if( ioctl( sock, SIOCGIFMTU, &ifr) < 0)
		throw SocketException("ioctl failed (getInterfaceAddress)");

	::close(sock);

	return ifr.ifr_mtu;
}

void NET::setMTU( const std::string& interface, int mtu)
{
	struct ifreq ifr;

	ifr.ifr_addr.sa_family = AF_INET;
	std::strncpy( ifr.ifr_name, interface.c_str(), sizeof(ifr.ifr_name));

	int sock = ::socket( AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if( sock < 0)
		throw SocketException("Couldn't create socket (setMTU)");

	ifr.ifr_mtu = mtu;

	if( ioctl( sock, SIOCSIFDSTADDR, &ifr) < 0)
		throw SocketException("ioctl failed (setMTU)");

	::close(sock);
}

std::string NET::getHardwareAddress( const std::string& interface, char separationChar)
{
	struct ifreq ifr;

	ifr.ifr_addr.sa_family = AF_INET;
	std::strncpy( ifr.ifr_name, interface.c_str(), sizeof(ifr.ifr_name));

	int sock = ::socket( AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if( sock < 0)
		throw SocketException("Couldn't create socket (getMTU)");

	if( ioctl( sock, SIOCGIFHWADDR, &ifr) < 0)
		throw SocketException("ioctl failed (getInterfaceAddress)");

	::close(sock);

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
