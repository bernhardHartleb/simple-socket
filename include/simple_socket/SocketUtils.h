#ifndef NET_SocketUtils_h__
#define NET_SocketUtils_h__

#include <cstdint>
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
}
#endif // NET_SocketUtils_h__
