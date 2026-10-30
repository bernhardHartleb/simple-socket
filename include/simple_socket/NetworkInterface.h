#ifndef NET_NetworkInterface_h__
#define NET_NetworkInterface_h__

#include "SimpleSocket.h"

#include <string>
#include <string_view>
#include <vector>
#include <cstdint>

namespace NET
{
	struct NetworkAddressInfo
	{
		std::string name;
		std::string address;
		std::string netmask;
		std::string broadcast;
		std::string destination;
		AddressFamily family;
		unsigned int flags;
		uint32_t scopeId = 0;
	};

	//! Return IPv4/IPv6 address entries for the local network interfaces.
	/*!
	 * One NetworkAddressInfo is returned per address, so an interface can
	 * occur multiple times when it has multiple IPv4/IPv6 addresses.
	 * Link-layer entries (AF_PACKET / AF_LINK) are ignored.
	 *
	 * \return interface address entries returned by getifaddrs()
	 */
	std::vector<NetworkAddressInfo> getNetworkAddresses();

	class NetworkInterface;

	//! Returns list of all network interfaces regardless of their state
	/*!
	 * This function does not understand interfaces with multiple addresses
	 * For example when using zeroconf (eth0, eth0:avahi) it would only list eth0
	 * In this case eth0 does not have an ip address yet and getInterfaceAddress("eth0") throws
	 */
	std::vector<NetworkInterface> getNetworkInterfaces();


	class NetworkInterface
	{
	public:
		NetworkInterface( std::string_view interface);

		std::string name() const { return interface; }

		//! Return the IPv4 address of the given network interface
		/*!
		* Call may throw if e.g. the interface it not up or not with
		* an IP address.
		*/
		std::string getInterfaceAddress() const;

		//! Set the IPv4 address of the given network interface
		/*!
		* Call may throw if unable to set IP address (e.g. non Ethernet
		* interface, etc.)
		*/
		void setInterfaceAddress( std::string_view address);

		//! Return the IPv4 broadcast address of the given network interface
		/*!
		* The IOCTL call SIOCGIFBRDADDR is used on the specified interface.
		* This method only works on AF_INET interfaces.
		* Returns empty string on failure.
		*
		* \return the broadcast address formatted as an ip address
		*/
		std::string getBroadcastAddress() const;

		//! Set the IPv4 broadcast address of the given network interface
		/*!
		* This call my throw if e.g. used on an interface that can not have
		* a BC address e.g. a bluetooth device
		*
		* \param address IPV4 address to configure the interface to
		*/
		void setBroadcastAddress( std::string_view address);

		//! Return the IPv4 netmask of the given network interface
		/*!
		* Returns empty string if the interface e.g. has no netmask
		* configured (e.g. if interface is down) or can't have one (e.g. bluetooth)
		*/
		std::string getNetmask() const;

		//! Set the IPv4 netmask of the given network interface
		/*!
		* This call may throw if the interface e.g. can not have a broadcast
		* address, is down, etc.
		*
		* \param address IPV4 address to set the netmask to
		*/
		void setNetmask( std::string_view address);

		//! Return the IPv4 destination address of the given network interface
		/*!
		* This call may throw if the interface e.g. has no destination address
		* configured (e.g. if interface is down) or can't have one (e.g. bluetooth)
		*/
		std::string getDestinationAddress() const;

		//! Set the IPv4 destination address of the given network interface
		/*!
		* Returns empty string if the interface e.g. can not have a destination
		* address, is down, etc.
		*
		* \param address IPV4 address to set the netmask to
		*/
		void setDestinationAddress( std::string_view address);

		//! Return the MTU (Maximum Transmission Unit) of the given network interface in bytes
		/*!
		* This call may throw.
		*/
		int getMTU() const;

		//! Set the MTU (Maximum Transmission Unit) of the given network interface in bytes
		/*!
		* This call may throw.
		*
		* \param mtu new mtu in bytes
		*/
		void setMTU( int mtu);

		//! Return the MAC address of the given network interfaces
		/*!
		* Returns empty string on failure.
		*/
		std::string getHardwareAddress() const;

	private:
		std::string interface;
	};
}
#endif // NET_NetworkInterface_h__
