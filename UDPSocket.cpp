#include "UDPSocket.h"
#include "SocketUtils.h"

#include <sys/types.h>		// for data types
#include <sys/socket.h>		// for socket(), connect(), send(), and recv()
#include <poll.h>			// for poll()
#include <arpa/inet.h>		// for inet_addr()
#include <netinet/in.h>		// for sockaddr_in

using namespace NET;

namespace {

void setBroadcast( int socket)
{
	// If this fails, we'll hear about it when we try to send. This will allow
	// systems that cannot broadcast to continue if they don't plan to broadcast
	int broadcastPermission = 1;
	setsockopt( socket,
		    SOL_SOCKET,
		    SO_BROADCAST,
		    (raw_type*)&broadcastPermission,
		    sizeof(broadcastPermission));
}

int groupAction( int socket, const std::string& multicastGroup, int action)
{
	struct ip_mreq multicastRequest;

	multicastRequest.imr_multiaddr.s_addr = inet_addr( multicastGroup.c_str());
	multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);
	return( setsockopt( socket,
			    IPPROTO_IP,
			    action,
			    (raw_type*)&multicastRequest,
			    sizeof(multicastRequest)));
}

} // namespace

UDPSocket::UDPSocket()
: InternetSocket( DATAGRAM, IPPROTO_UDP)
{
	setBroadcast(m_socket);
}

UDPSocket::UDPSocket( unsigned short localPort)
: InternetSocket( DATAGRAM, IPPROTO_UDP)
{
	bind(localPort);
	setBroadcast(m_socket);
}

UDPSocket::UDPSocket( const std::string& localAddress, unsigned short localPort)
: InternetSocket( DATAGRAM, IPPROTO_UDP)
{
	bind( localAddress, localPort);
	setBroadcast(m_socket);
}

void UDPSocket::sendTo( const void* buffer, size_t len, const std::string& foreignAddress, unsigned short foreignPort)
{
	sockaddr_in destAddr;
	fillAddr( foreignAddress, foreignPort, destAddr);

	// Write out the whole buffer as a single message.
	if( sendto( m_socket, (const raw_type*)buffer, len, 0, (sockaddr*)&destAddr, sizeof(destAddr)) != (int)len)
		throw SocketException("Send failed (sendto)");
}

int UDPSocket::receiveFrom( void* buffer, size_t len, std::string& sourceAddress, unsigned short& sourcePort)
{
	sockaddr_in clientAddr;
	socklen_t addrLen = sizeof(clientAddr);

	int ret = recvfrom( m_socket, (raw_type*)buffer, len, 0, (sockaddr*)&clientAddr, &addrLen);
	if( ret < 0)
		throw SocketException("Receive failed (recvfrom)");

	sourceAddress = inet_ntoa( clientAddr.sin_addr);
	sourcePort = ntohs( clientAddr.sin_port);

	return ret;
}

int UDPSocket::receiveFrom( void* buffer, size_t len, std::string& sourceAddress, unsigned short& sourcePort, int timeout)
{
	struct pollfd poll;
	poll.fd = m_socket;
	poll.events = POLLIN | POLLPRI | POLLRDHUP;

	int ret = ::poll( &poll, 1, timeout);

	if( poll.revents & POLLRDHUP)
	{
		m_peerDisconnected = true;
		return 0;
	}

	if( ret == 0) return 0;
	if( ret < 0)  throw SocketException("Poll failed (receive)");

	sockaddr_in clientAddr;
	socklen_t addrLen = sizeof(clientAddr);

	ret = recvfrom( m_socket, (raw_type*)buffer, len, 0, (sockaddr*)&clientAddr, &addrLen);
	if( ret < 0)
		throw SocketException("Receive failed (recvfrom)");

	sourceAddress = inet_ntoa( clientAddr.sin_addr);
	sourcePort = ntohs( clientAddr.sin_port);

	return ret;
}

void UDPSocket::setMulticastTTL( unsigned char multicastTTL)
{
	if( setsockopt( m_socket,
	                IPPROTO_IP,
	                IP_MULTICAST_TTL,
	                (raw_type*)&multicastTTL,
	                sizeof(multicastTTL)) < 0)
		throw SocketException("Multicast set TTL failed (setsockopt)");
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
