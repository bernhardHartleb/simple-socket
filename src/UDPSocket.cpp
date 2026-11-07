#include "UDPSocket.h"
#include "SockAddress.h"
#include "TempFailure.h"
#include "PollHelper.h"

#include <sys/socket.h>
#include <arpa/inet.h>

using namespace NET;

namespace {

void setBroadcast( int socket)
{
	// If this fails, we'll hear about it when we try to send. This will allow
	// systems that cannot broadcast to continue if they don't plan to broadcast
	const int value = 1;
	::setsockopt( socket, SOL_SOCKET, SO_BROADCAST, &value, sizeof(value));
}

int groupAction( int socket, const std::string& multicastGroup, int action)
{
	struct ip_mreq request;
	request.imr_multiaddr.s_addr = inet_addr( multicastGroup.c_str());
	request.imr_interface.s_addr = htonl(INADDR_ANY);

	return ::setsockopt( socket, IPPROTO_IP, action, &request, sizeof(request));
}

} // namespace

UDPSocket::UDPSocket( AddressFamily family)
: InternetSocket( family, SOCK_DGRAM, IPPROTO_UDP)
{
	setBroadcast(m_socket);
}

UDPSocket UDPSocket::adoptNativeHandle( int sockfd)
{
	validateSocket( sockfd, SOCK_DGRAM, IPPROTO_UDP);
	return UDPSocket( AdoptSocketTag{}, sockfd, SOCK_DGRAM);
}

IOResult UDPSocket::sendTo( const void* buffer, size_t len, std::string_view foreignAddress, unsigned short foreignPort)
{
	SockAddress addr( m_family, foreignAddress, foreignPort);
	return SimpleSocket::sendTo( buffer, len, addr);
}

IOResult UDPSocket::receiveFrom( void* buffer, size_t len, std::string& sourceAddress, unsigned short& sourcePort)
{
	SockAddress addr;
	IOResult res = SimpleSocket::receiveFrom( buffer, len, addr);

	if( res.status == IOStatus::Success)
	{
		sourceAddress = addr.address();
		sourcePort = addr.port();
	}
	return res;
}

IOResult UDPSocket::timedReceiveFrom( void* buffer, size_t len, std::string& sourceAddress, unsigned short& sourcePort, int timeout)
{
	return detail::timedReceiveImpl( m_socket, m_connectionOriented, timeout,
		[&] { return receiveFrom( buffer, len, sourceAddress, sourcePort); });
}

void UDPSocket::setMulticastTTL( unsigned char multicastTTL)
{
	const int value = multicastTTL;
	if( ::setsockopt( m_socket, IPPROTO_IP, IP_MULTICAST_TTL, &value, sizeof(value)) < 0)
		throw SocketException("Multicast set TTL failed (setsockopt)");
}

void UDPSocket::setMulticastInterfaceAddr( const std::string& address)
{
	struct in_addr iface;
	if( inet_aton( address.c_str(), &iface) == 0)
		throw SocketException("Invalid IPv4 address", false);

	if( ::setsockopt( m_socket, IPPROTO_IP, IP_MULTICAST_IF, &iface, sizeof(iface)) < 0)
		throw SocketException("Multicast set Interface Address failed (setsockopt)");
}

void UDPSocket::joinGroup( const std::string& multicastGroup)
{
	if( groupAction( m_socket, multicastGroup, IP_ADD_MEMBERSHIP) < 0)
		throw SocketException("Multicast group join failed (setsockopt)");
}

void UDPSocket::leaveGroup( const std::string& multicastGroup)
{
	if( groupAction( m_socket, multicastGroup, IP_DROP_MEMBERSHIP) < 0)
		throw SocketException("Multicast group leave failed (setsockopt)");
}
