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
	 * \param protocol protocol of service to resolve. Default is "tcp". To
	 * match to any protocol, pass an empty string ("").
	 * \return port number fetched from the database. 0 if no match found.
	 */
	uint16_t resolveService( const std::string& service, const std::string& protocol = "tcp");

	//! Return a list of available network interfaces
	std::vector<std::string> getNetworkInterfaces();

	//! Return the IPv4 address of the given network interface
	/*!
	 * Call may throw if e.g. the interface it not up or not with
	 * an IP address.
	 */
	std::string getInterfaceAddress( const std::string& interface);

	//! Set the IPv4 address of the given network interface
	/*!
	 * Call may throw if unable to set IP address (e.g. non Ethernet
	 * interface, etc.)
	 */
	void setInterfaceAddress( const std::string& interface, const std::string& address);

	//! Return the IPv4 broadcast address of the given network interface
	/*!
	 * The IOCTL call SIOCGIFBRDADDR is used on the specified interface.
	 * This method only works on AF_INET interfaces, it therefore works only on
	 * interfaces returned by ifconfig or getNetworkInterfaces().
	 * 
	 * The call may throw if e.g no BC address is configured or an interface
	 * is used that can not have a broadcast address (e.g. bluetooth).
	 *
	 * \param interface the interface to query for its broadcast address
	 * \return the broadcast address formatted as an ip address
	 */
	std::string getBroadcastAddress( const std::string& interface);

	//! Set the IPv4 broadcast address of the given network interface
	/*!
	 * This call my throw if e.g. used on an interface that can not have
	 * a BC address e.g. a bluetooth device
	 * 
	 * \param interface interface name
	 * \param address IPV4 address to configure the interface to
	 */
	void setBroadcastAddress( const std::string& interface, const std::string& address);

	//! Return the IPv4 netmask of the given network interface
	/*!
	 * This call may throw if the interface e.g. has no netmask
	 * configured (e.g. if interface is down) or can't have one (e.g. bluetooth)
	 * 
	 * \param interface interface name
	 */
	std::string getNetmask( const std::string& interface);

	//! Set the IPv4 netmask of the given network interface
	/*!
	 * This call may throw if the interface e.g. can not have a broadcast
	 * address, is down, etc.
	 * 
	 * \param interface interface name
	 * \param address IPV4 address to set the netmask to
	 */
	void setNetmask( const std::string& interface, const std::string& address);

	//! Return the IPv4 destination address of the given network interface
	/*!
	 * This call may throw if the interface e.g. has no destination address
	 * configured (e.g. if interface is down) or can't have one (e.g. bluetooth)
	 * 
	 * \param interface interface name
	 */
	std::string getDestinationAddress( const std::string& interface);

	//! Set the IPv4 destination address of the given network interface
	/*!
	 * This call may throw if the interface e.g. can not have a destination
	 * address, is down, etc.
	 * 
	 * \param interface interface name
	 * \param address IPV4 address to set the netmask to
	 */
	void setDestinationAddress( const std::string& interface, const std::string& address);

	//! Return the MTU (Maximum Transmission Unit) of the given network interface in bytes
	/*!
	 * This call may throw.
	 * 
	 * \param interface interface name
	 */
	int getMTU( const std::string& interface);

	//! Set the MTU (Maximum Transmission Unit) of the given network interface in bytes
	/*!
	 * This call may throw.
	 * 
	 * \param interface interface name
	 * \param mtu new mtu in bytes
	 */
	void setMTU( const std::string& interface, int mtu);

	//! Return the MAC address of the given network interfaces
	/*!
	 * This call may throw if the interface does not have a MAC address.
	 * 
	 * \param interface interface name
	 * \param separationChar char used to separate indivdual bytes
	 */
	std::string getHardwareAddress( const std::string& interface, char separationChar = ':');
}
#endif // NET_SocketUtils_h__
