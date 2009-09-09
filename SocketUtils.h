#ifndef NET_SocketUtils_h__
#define NET_SocketUtils_h__

#include <vector>
#include <string>

struct sockaddr_in;

// TODO change network interface name
// TODO time of last received packet (SIOCGSTAMP / SIOCGSTAMPNS)
// TODO routing table maintenance ( SIOCADDRT / SIOCDELRT / SIOCRTMSG )
// TODO arp table maintenance ( SIOCDARP / SIOCGARP / SIOCSARP )

namespace NET
{
	/*!
		Function to fill an address structure with the given address and port number.
		If the given address is not a valid IPv4 address, it will be resolved
		by hostname or DNS lookup. addr will be unchanged if this resolve fails.

		\param address IPv4 domain name or address
		\param port IP port number to fill in
		\param addr address structure to fill
	*/
	void fillAddr( const std::string& address, unsigned short port, sockaddr_in& addr);

	/*!
		Resolve the specified service for the specified protocol to the
		corresponding port number in host byte order.

		\param service service to resolve (e.g., "http")
		\param protocol protocol of service to resolve. Default is "tcp".
	*/
	unsigned short resolveService( const std::string& service, const std::string& protocol = "tcp");

	//std::string resolveHostname( const std::string& hostname);

	//! Return a list of available network interfaces
	std::vector<std::string> getNetworkInterfaces();

	//! Return the IPv4 address of a network interface
	std::string getInterfaceAddress( const std::string& interface);
	void setInterfaceAddress( const std::string& interface, const std::string& address);

	//! retrieve the broadcast address for an interface
	/*!
	 * getBroadcastAdress tries to retrieve the broadcast address of an interface.
	 * It uses the IOCTL call SIOCGIFBRDADDR on the specified interface. This method only
	 * works on AF_INET interfaces, it therefore works only on interfaces shown by ifconfig
	 * or by getNetworkInterfaces().
	 *
	 * \param interface the interface to query for its broadcast address
	 * \return the broadcast address formatted as an ip address
	 */
	std::string getBroadcastAddress( const std::string& interface);
	void setBroadcastAddress( const std::string& interface, const std::string& address);

	std::string getNetmask( const std::string& interface);
	void setNetmask( const std::string& interface, const std::string& address);

	std::string getDestinationAddress( const std::string& interface);
	void setDestinationAddress( const std::string& interface, const std::string& address);

	int getMTU( const std::string& interface);
	void setMTU( const std::string& interface, int mtu);

	std::string getHardwareAddress( const std::string& interface, char separationChar);

}
#endif // NET_SocketUtils_h__
