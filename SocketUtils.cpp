#include "SocketUtils.h"
#include "SimpleSocket.h"

#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netdb.h>		// for gethostbyname()
#include <cstring>		// for memset()
#include <cstdlib>		// for ntohs() and atoi()

#include <sstream>
#include <string>
#include <vector>

using namespace NET;

void NET::fillAddr( const std::string& address, unsigned short port, sockaddr_in& addr)
{
	std::memset( &addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;

	hostent* host = gethostbyname( address.c_str());
	if( host == NULL)
	{
		// strerror() will not work for gethostbyname() and hstrerror()
		// is supposedly obsolete
		//throw SocketException("Failed to resolve name (gethostbyname())");
		throw; // temporary workaround
	}
	addr.sin_addr.s_addr = *( (ulong*)host->h_addr_list[0]);

	// Assign port in network byte order
	addr.sin_port = htons(port);
}

unsigned short NET::resolveService( const std::string& service, const std::string& protocol)
{
	// Structure containing service information
	struct servent* serv = getservbyname( service.c_str(), protocol.c_str());

	if(!serv)
		// Service is port number
		return std::atoi( service.c_str());
	else
		// Found port (network byte order) by name
		return ntohs( serv->s_port);
}

std::vector<std::string> NET::getNetworkInterfaces()
{
	std::vector<std::string> ret;
	struct if_nameindex* index = if_nameindex();
	for( int i = 0; index[i].if_index != 0; ++i)
		ret.push_back( std::string(index[i].if_name));
	if_freenameindex(index);
	return ret;
}

std::string NET::getInterfaceAddress( const std::string& interface)
{
	struct ifreq ifr;

	ifr.ifr_addr.sa_family = AF_INET;
	std::strncpy( ifr.ifr_name, interface.c_str(), sizeof(ifr.ifr_name));

	int sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if( sock < 0)
		throw SocketException("Couldn't create socket (getInterfaceAddress)");

	if( ioctl( sock, SIOCGIFADDR, &ifr) < 0)
		throw SocketException("ioctl failed (getInterfaceAddress)");

	close(sock);

	return inet_ntoa( reinterpret_cast<sockaddr_in*>(&ifr.ifr_addr)->sin_addr);
}

void NET::setInterfaceAddress( const std::string& interface, const std::string& addr)
{
	struct ifreq ifr;

	ifr.ifr_addr.sa_family = AF_INET;
	std::strncpy( ifr.ifr_name, interface.c_str(), sizeof(ifr.ifr_name));

	int sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if( sock < 0)
		throw SocketException("Couldn't create socket (setInterfaceAddress)");

	inet_aton( addr.c_str(), &(reinterpret_cast<sockaddr_in*>(&ifr.ifr_addr)->sin_addr));
	if( ioctl( sock, SIOCSIFADDR, &ifr) < 0)
		throw SocketException("ioctl failed (setInterfaceAddress)");
}

std::string NET::getBroadcastAddress( const std::string& interface)
{
	struct ifreq ifr;

	ifr.ifr_addr.sa_family = AF_INET;
	std::strncpy( ifr.ifr_name, interface.c_str(), sizeof(ifr.ifr_name));

	int sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if( sock < 0)
		throw SocketException("Couldn't create socket (getBroadcastAddress)");
	
	if( ioctl( sock, SIOCGIFBRDADDR, &ifr) < 0)
		throw SocketException("ioctl failed (getInterfaceAddress)");

	close(sock);

	return inet_ntoa( reinterpret_cast<sockaddr_in*>(&ifr.ifr_broadaddr)->sin_addr);
}

void NET::setBroadcastAddress( const std::string& interface, const std::string& addr)
{
	struct ifreq ifr;

	ifr.ifr_addr.sa_family = AF_INET;
	std::strncpy( ifr.ifr_name, interface.c_str(), sizeof(ifr.ifr_name));

	int sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if( sock < 0)
		throw SocketException("Couldn't create socket (setBroadcastAddress)");

	inet_aton( addr.c_str(), &(reinterpret_cast<sockaddr_in*>(&ifr.ifr_broadaddr)->sin_addr));

	if( ioctl( sock, SIOCSIFBRDADDR, &ifr) < 0)
		throw SocketException("ioctl failed (setBroadcastAddress)");
}

std::string NET::getNetmask( const std::string& interface)
{
	struct ifreq ifr;

	ifr.ifr_addr.sa_family = AF_INET;
	std::strncpy( ifr.ifr_name, interface.c_str(), sizeof(ifr.ifr_name));

	int sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if( sock < 0)
		throw SocketException("Couldn't create socket (getBroadcastAddress)");
	
	if( ioctl( sock, SIOCGIFNETMASK, &ifr) < 0)
		throw SocketException("ioctl failed (getNetmask)");

	close(sock);

	return inet_ntoa( reinterpret_cast<sockaddr_in*>(&ifr.ifr_netmask)->sin_addr);
}

void NET::setNetmask( const std::string& interface, const std::string& addr)
{
	struct ifreq ifr;

	ifr.ifr_addr.sa_family = AF_INET;
	std::strncpy( ifr.ifr_name, interface.c_str(), sizeof(ifr.ifr_name));

	int sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if( sock < 0)
		throw SocketException("Couldn't create socket (setNetmask)");

	inet_aton( addr.c_str(), &(reinterpret_cast<sockaddr_in*>(&ifr.ifr_netmask)->sin_addr));
	if( ioctl( sock, SIOCSIFNETMASK, &ifr) < 0)
		throw SocketException("ioctl failed (setNetmask)");
}

std::string NET::getDestinationAddress( const std::string& interface) 
{
	struct ifreq ifr;

	ifr.ifr_addr.sa_family = AF_INET;
	std::strncpy( ifr.ifr_name, interface.c_str(), sizeof(ifr.ifr_name));

	int sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if( sock < 0)
		throw SocketException("Couldn't create socket (getBroadcastAddress)");
	
	if( ioctl( sock, SIOCGIFDSTADDR, &ifr) < 0)
		throw SocketException("ioctl failed (getDestinationAddress)");

	close(sock);

	return inet_ntoa( reinterpret_cast<sockaddr_in*>(&ifr.ifr_dstaddr)->sin_addr);
}

void NET::setDestinationAddress( const std::string& interface, const std::string& addr)
{
	struct ifreq ifr;

	ifr.ifr_addr.sa_family = AF_INET;
	std::strncpy( ifr.ifr_name, interface.c_str(), sizeof(ifr.ifr_name));

	int sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if( sock < 0)
		throw SocketException("Couldn't create socket (setDestinationAddress)");

	inet_aton( addr.c_str(), &(reinterpret_cast<sockaddr_in*>(&ifr.ifr_dstaddr)->sin_addr));
	if( ioctl( sock, SIOCSIFDSTADDR, &ifr) < 0)
		throw SocketException("ioctl failed (setDestinationAddress)");
}

int NET::getMTU( const std::string& interface)
{
	struct ifreq ifr;
	
	ifr.ifr_addr.sa_family = AF_INET;
	std::strncpy( ifr.ifr_name, interface.c_str(), sizeof(ifr.ifr_name));

	int sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if( sock < 0)
		throw SocketException("Couldn't create socket (getMTU)");

	if( ioctl( sock, SIOCGIFMTU, &ifr) < 0)
		throw SocketException("ioctl failed (getInterfaceAddress)");

	close(sock);

	return ifr.ifr_mtu;
}

void NET::setMTU( const std::string& interface, int mtu)
{
	struct ifreq ifr;

	ifr.ifr_addr.sa_family = AF_INET;
	std::strncpy( ifr.ifr_name, interface.c_str(), sizeof(ifr.ifr_name));

	int sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if( sock < 0)
		throw SocketException("Couldn't create socket (setMTU)");

	ifr.ifr_mtu = mtu;
	if( ioctl( sock, SIOCSIFDSTADDR, &ifr) < 0)
		throw SocketException("ioctl failed (setMTU)");
}

std::string NET::getHardwareAddress( const std::string& interface, char separationChar)
{
	struct ifreq ifr;
	
	ifr.ifr_addr.sa_family = AF_INET;
	std::strncpy( ifr.ifr_name, interface.c_str(), sizeof(ifr.ifr_name));

	int sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if( sock < 0)
		throw SocketException("Couldn't create socket (getMTU)");

	if( ioctl( sock, SIOCGIFHWADDR, &ifr) < 0)
		throw SocketException("ioctl failed (getInterfaceAddress)");

	close(sock);

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
	for( int i = 0; i <= 3; ++i)
		str << std::hex << static_cast<uint8_t>(ifr.ifr_addr.sa_data[i]) << separationChar;
	str << std::hex << static_cast<uint8_t>(ifr.ifr_addr.sa_data[4]);
	return str.str();
}

