#ifndef NET_SocketUtils_h__
#define NET_SocketUtils_h__

#include <string>

struct sockaddr_in;

namespace NET
{
	//! Function to fill an address structure with the given address and port
	void fillAddr( const std::string& address, unsigned short port, sockaddr_in& addr);

	/*!
		Resolve the specified service for the specified protocol to the
		corresponding port number in host byte order
		\param service service to resolve (e.g., "http")
		\param protocol protocol of service to resolve. Default is "tcp".
	*/
	unsigned short resolveService( const std::string& service, const std::string& protocol = "tcp");

	//std::string resolveHostname( const std::string& hostname);

	//! Return the IPv4 address of a network interface
	std::string getInterfaceAddress( const std::string& interface);

	//! retrieve the broadcast address for an interface
	/*!
	 * getBroadcastAdress tries to retrieve the broadcast address of an interface.
	 * It uses the IOCTL call SIOCGIFBRDADDR on the specified interface. This method only
	 * works on AF_INET interfaces, it therefore works only on interfaces shown by ifconfig.
	 *
	 * \param interface the interface to query for its broadcast address
	 * \return the broadcast address formatted as an ip address
	 */
	std::string getBroadcastAddress( const std::string& interface);
}
#endif // NET_SocketUtils_h__
