#include "InternetSocket.h"
#include "TempFailure.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include <fcntl.h>
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

int InternetSocket::timedConnect( std::string_view foreignAddress, unsigned short foreignPort, int timeout)
{
	sockaddr_in addr;
	fillAddress( foreignAddress, foreignPort, addr);

	// Set O_NONBLOCK
	int flags_before = ::fcntl( m_socket, F_GETFL, 0);
	if( flags_before < 0)
		throw SocketException("timedConnect failed (fcntl)");
	::fcntl( m_socket, F_SETFL, flags_before | O_NONBLOCK);

	int ret = 0;
	// Start connecting (asynchronously)
	if( ::connect( m_socket, (sockaddr*) &addr, sizeof(addr)) == 0)
		{ ret = 1; goto exit; }

	// Did connect return an unexpected error?
	if( (errno != EWOULDBLOCK) && (errno != EINPROGRESS))
		{ ret = -1; goto exit; }

	// Wait for the connection to complete.
	struct pollfd poll;
	poll.fd = m_socket;
	poll.events = POLLOUT;

	ret = TEMP_FAILURE_RETRY (::poll( &poll, 1, timeout));

	if( ret < 0) goto exit;
	if( ret == 0) // Did poll timeout?
	{
		errno = ETIMEDOUT;
	}
	if( ret > 0) // If poll succeeded, make sure there is no error
	{
		int error = 0; socklen_t len = sizeof(error);
		int opt = ::getsockopt( m_socket, SOL_SOCKET, SO_ERROR, &error, &len);
		if( opt == 0) errno = error;
		if( error != 0) ret = -1;
	}

  exit:
	// Restore original flags
	::fcntl( m_socket, F_SETFL, flags_before);

	if( ret < 0)
		throw SocketException("timedConnect failed (connect)");
	if( ret > 0)
		m_peerDisconnected = false;

	return ret;
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
