#include "InternetSocket.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>

using namespace NET;

InternetSocket::InternetSocket( int type, int protocol)
: SimpleSocket( INTERNET, type, protocol)
{}

InternetSocket::InternetSocket( int sockfd)
: SimpleSocket(sockfd) {}

void InternetSocket::connect( std::string_view foreignAddress, unsigned short foreignPort)
{
	sockaddr_in addr;
	fillAddress( foreignAddress, foreignPort, addr);

	if( ::connect( m_socket, (sockaddr*) &addr, sizeof(addr)) < 0)
		throw SocketException("Connect failed (connect)");

	m_peerDisconnected = false;
}

void InternetSocket::bind( unsigned short localPort /* = 0 */)
{
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(localPort);

	if( ::bind( m_socket, (sockaddr*) &addr, sizeof(addr)) < 0)
		throw SocketException("Set of local port failed (bind)");
}

void InternetSocket::bind( std::string_view localAddress, unsigned short localPort /* = 0 */)
{
	sockaddr_in addr;
	fillAddress( localAddress, localPort, addr);

	if( ::bind( m_socket, (sockaddr*) &addr, sizeof(addr)) < 0)
		throw SocketException("Set of local address and port failed (bind)");
}

std::string InternetSocket::getLocalAddress() const
{
	sockaddr_in addr;
	socklen_t addr_len = sizeof(addr);

	if( getsockname( m_socket, (sockaddr*) &addr, &addr_len) < 0)
		throw SocketException("Fetch of local address failed (getsockname)");

	return inet_ntoa( addr.sin_addr);
}

unsigned short InternetSocket::getLocalPort() const
{
	sockaddr_in addr;
	socklen_t addr_len = sizeof(addr);

	if( getsockname( m_socket, (sockaddr*) &addr, &addr_len) < 0)
		throw SocketException("Fetch of local port failed (getsockname)");

	return ntohs( addr.sin_port);
}

std::string InternetSocket::getForeignAddress() const
{
	sockaddr_in addr;
	socklen_t addr_len = sizeof(addr);

	if( getpeername( m_socket, (sockaddr*) &addr, &addr_len) < 0)
		throw SocketException("Fetch of foreign address failed (getpeername)");

	return inet_ntoa( addr.sin_addr);
}

unsigned short InternetSocket::getForeignPort() const
{
	sockaddr_in addr;
	socklen_t addr_len = sizeof(addr);

	if( getpeername( m_socket, (sockaddr*) &addr, &addr_len) < 0)
		throw SocketException("Fetch of foreign port failed (getpeername)");

	return ntohs( addr.sin_port);
}

void InternetSocket::fillAddress( std::string_view address, unsigned short port, sockaddr_in& addr)
{
	const int IP_MAXSIZE = 20;
	const int len = address.length();

	// needed space is size plus null character
	if( len >= IP_MAXSIZE)
		throw SocketException("IPv4 address is too long", false);

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	char buf[IP_MAXSIZE];
	std::memcpy( buf, address.data(), len);
	buf[len] = 0;

	if( inet_aton( buf, &addr.sin_addr) == 0)
		throw SocketException("Unable to parse IPv4 address");
}
