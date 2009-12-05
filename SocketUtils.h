#ifndef NET_SocketUtils_h__
#define NET_SocketUtils_h__

#include <vector>
#include <string>

namespace NET
{
	/*!
	 * Resolve the specified hostname to a standard IPv4 address.
	 * If the operating system doesn't know the hostname yet this means a DNS lookup.
	 *
	 * \param hostname domain name
	 * \return resolved hostname as standard IPv4 address
	 */
	std::string resolveHostname( const std::string& hostname);

	/*!
	 * Resolve the specified service for the specified protocol to the
	 * corresponding port number in host byte order.
	 *
	 * \param service service to resolve (e.g. "http")
	 * \param protocol protocol of service to resolve. Default is "tcp".
	 */
	unsigned short resolveService( const std::string& service, const std::string& protocol = "tcp");

	//! Return a list of available network interfaces
	std::vector<std::string> getNetworkInterfaces();

	//! Return the IPv4 address of the given network interface
	std::string getInterfaceAddress( const std::string& interface);

	//! Set the IPv4 address of the given network interface
	void setInterfaceAddress( const std::string& interface, const std::string& address);

	//! Return the IPv4 broadcast address of the given network interface
	/*!
	 * The IOCTL call SIOCGIFBRDADDR is used on the specified interface.
	 * This method only works on AF_INET interfaces, it therefore works only on
	 * interfaces returned by ifconfig or getNetworkInterfaces().
	 *
	 * \param interface the interface to query for its broadcast address
	 * \return the broadcast address formatted as an ip address
	 */
	std::string getBroadcastAddress( const std::string& interface);

	//! Set the IPv4 broadcast address of the given network interface
	void setBroadcastAddress( const std::string& interface, const std::string& address);

	//! Return the IPv4 netmask of the given network interface
	std::string getNetmask( const std::string& interface);

	//! Set the IPv4 netmask of the given network interface
	void setNetmask( const std::string& interface, const std::string& address);

	//! Return the IPv4 destination address of the given network interface
	std::string getDestinationAddress( const std::string& interface);

	//! Set the IPv4 destination address of the given network interface
	void setDestinationAddress( const std::string& interface, const std::string& address);

	//! Return the MTU (Maximum Transmission Unit) of the given network interface in bytes
	int getMTU( const std::string& interface);

	//! Set the MTU (Maximum Transmission Unit) of the given network interface in bytes
	void setMTU( const std::string& interface, int mtu);

	//! Return the MAC address of the given network interfaces
	std::string getHardwareAddress( const std::string& interface, char separationChar = ':');
}
#endif // NET_SocketUtils_h__
