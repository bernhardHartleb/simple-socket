#include "SocketUtils.h"

#include "SimpleSocket.h"

#include <sys/types.h>		// for data types
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>		// for gethostbyname()
#include <cstring>		// for memset()
#include <cstdlib>		// for ntohs() and atoi()

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

/*std::vector<std::string> NET::getNetworkInterfaces() {

}*/

#define inaddrr(x) (*(struct in_addr *) &ifr->x[sizeof sa.sin_port])

std::string NET::getInterfaceAddress( const std::string& interface) {
	struct ifreq* ifr;
	struct ifreq ifrr;
	struct sockaddr_in sa;

	ifr = &ifrr;
	ifrr.ifr_addr.sa_family = AF_INET;
	strncpy(ifrr.ifr_name, interface.c_str(), sizeof(ifrr.ifr_name));

	int sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if( sock < 0)
		throw SocketException("Couldn't create socket (getInterfaceAddress)");

	if( ioctl( sock, SIOCGIFADDR, ifr) < 0)
		throw SocketException("ioctl failed (getInterfaceAddress");
	
	return inet_ntoa(inaddrr(ifr_addr.sa_data));
}

std::string NET::getBroadcastAddress( const std::string& interface) {
	
}
