#include "UDPSocket.h"
#include "SockAddress.h"
#include "TempFailure.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>

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

void UDPSocket::sendTo( const void* buffer, size_t len, std::string_view foreignAddress, unsigned short foreignPort)
{
	SockAddress addr( m_family, foreignAddress, foreignPort);

	int sent = TEMP_FAILURE_RETRY (::sendto( m_socket, (const raw_type*)buffer, len, 0, addr.data(), addr.length()));

	// Write out the whole buffer as a single message
	if( sent != (int)len)
		throw SocketException("Send failed (sendto)");
}

int UDPSocket::receiveFrom( void* buffer, size_t len, std::string& sourceAddress, unsigned short& sourcePort)
{
	SockAddress addr;

	int ret = TEMP_FAILURE_RETRY (::recvfrom( m_socket, (raw_type*)buffer, len, 0, addr.data(), addr.lengthPtr()));
	if( ret < 0)
		throw SocketException("Receive failed (recvfrom)");

	sourceAddress = addr.address();
	sourcePort = addr.port();

	return ret;
}

int UDPSocket::timedReceiveFrom( void* buffer, size_t len, std::string& sourceAddress, unsigned short& sourcePort, int timeout)
{
	struct pollfd poll;
	poll.fd = m_socket;
	poll.events = POLLIN | POLLPRI | POLLRDHUP;

	int ret = TEMP_FAILURE_RETRY (::poll( &poll, 1, timeout));

	if( ret == 0) return 0;
	if( ret < 0)  throw SocketException("timedReceive failed (poll)");

	if( poll.revents & POLLRDHUP || poll.revents & POLLHUP)
		m_peerDisconnected = true;

	if( poll.revents & POLLIN || poll.revents & POLLPRI)
		return receiveFrom( buffer, len, sourceAddress, sourcePort);

	if( poll.revents & POLLERR)
		throw SocketException("timedReceive failed (poll)");

	return 0;
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
