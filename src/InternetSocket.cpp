#include "InternetSocket.h"
#include "SockAddress.h"

#include <sys/socket.h>

using namespace NET;

namespace {

AddressFamily socketAddressFamily(int fd)
{
	SockAddress addr;
	if( ::getsockname( fd, addr.data(), addr.lengthPtr()) < 0)
		throw SocketException("Unable to read socket name (getsockname)");

	switch (addr.family())
	{
		case AF_INET: return AddressFamily::IPv4;
		case AF_INET6: return AddressFamily::IPv6;
		default:
			throw SocketException("Unsupported Internet socket family", false);
	}
}

}; // namespace

InternetSocket::InternetSocket( AddressFamily family, int type, int protocol)
: SimpleSocket( SockAddress::nativeFamily(family), type, protocol)
, m_family( family)
{}

InternetSocket::InternetSocket( AdoptSocketTag tag, int sockfd, int type)
: SimpleSocket( tag, sockfd, type)
, m_family( socketAddressFamily(sockfd))
{}

void InternetSocket::setReuseAddress( bool enable)
{
	const int value = enable;
	if( ::setsockopt( m_socket, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)) < 0)
		throw SocketException("Failed to set address reuse (setsockopt)");
}

ConnectResult InternetSocket::connect( std::string_view foreignAddress, unsigned short foreignPort)
{
	SockAddress addr( m_family, foreignAddress, foreignPort);
	return SimpleSocket::connect( addr);
}

ConnectResult InternetSocket::timedConnect( std::string_view foreignAddress, unsigned short foreignPort, int timeout)
{
	SockAddress addr( m_family, foreignAddress, foreignPort);
	return SimpleSocket::timedConnect( addr, timeout);
}

void InternetSocket::bind( unsigned short localPort /* = 0 */)
{
	SockAddress addr( m_family, localPort);

	if( ::bind( m_socket, addr.data(), addr.length()) < 0)
		throw SocketException("Set of local port failed (bind)");
}

void InternetSocket::bind( std::string_view localAddress, unsigned short localPort /* = 0 */)
{
	SockAddress addr( m_family, localAddress, localPort);

	if( ::bind( m_socket, addr.data(), addr.length()) < 0)
		throw SocketException("Set of local address and port failed (bind)");
}

std::string InternetSocket::getLocalAddress() const
{
	SockAddress addr;

	if( ::getsockname( m_socket, addr.data(), addr.lengthPtr()) < 0)
		throw SocketException("Fetch of local address failed (getsockname)");

	return addr.address();
}

unsigned short InternetSocket::getLocalPort() const
{
	SockAddress addr;

	if( ::getsockname( m_socket, addr.data(), addr.lengthPtr()) < 0)
		throw SocketException("Fetch of local port failed (getsockname)");

	return addr.port();
}

std::string InternetSocket::getForeignAddress() const
{
	SockAddress addr;

	if( ::getpeername( m_socket, addr.data(), addr.lengthPtr()) < 0)
	{
		if( errno == ENOTCONN) return {};
		throw SocketException("Fetch of foreign address failed (getpeername)");
	}
	return addr.address();
}

unsigned short InternetSocket::getForeignPort() const
{
	SockAddress addr;

	if( ::getpeername( m_socket, addr.data(), addr.lengthPtr()) < 0)
	{
		if( errno == ENOTCONN) return 0;
		throw SocketException("Fetch of foreign port failed (getpeername)");
	}
	return addr.port();
}
