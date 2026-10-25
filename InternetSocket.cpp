#include "InternetSocket.h"
#include "TempFailure.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include <fcntl.h>
#include <cstring>

using namespace NET;

namespace {

class NonBlockingGuard
{
public:
	explicit NonBlockingGuard(int fd) : m_fd(fd)
	{
		m_flags = ::fcntl( m_fd, F_GETFL, 0);
		if( m_flags < 0)
			throw SocketException("Unable to get file status (fcntl)");
		if( ::fcntl( m_fd, F_SETFL, m_flags | O_NONBLOCK) < 0)
			throw SocketException("Unable to set non-blocking mode (fcntl)");
	}

	~NonBlockingGuard()
	{
		const int savedErrno = errno;
		::fcntl( m_fd, F_SETFL, m_flags);
		errno = savedErrno;
	}

	NonBlockingGuard(const NonBlockingGuard&) = delete;
	NonBlockingGuard& operator=(const NonBlockingGuard&) = delete;

private:
	int m_fd;
	int m_flags;
};

}; // namespace

InternetSocket::InternetSocket( SocketType type, int protocol)
: SimpleSocket( INTERNET, type, protocol)
{}

InternetSocket::InternetSocket( int sockfd)
: SimpleSocket(sockfd)
{}

void InternetSocket::setReuseAddress( bool enable)
{
	const int value = enable;
	if( ::setsockopt( m_socket, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)) < 0)
		throw SocketException("Failed to set address reuse (setsockopt)");
}

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

	// Set O_NONBLOCK and restore blocking state on destruction
	NonBlockingGuard nonBlocking( m_socket);

	const auto throwError = []() {
		throw SocketException("timedConnect failed (connect)");
	};

	// Asynchronous connection attempt
	int ret = ::connect( m_socket, (sockaddr*) &addr, sizeof(addr));
	if( ret == 0) { // Already connected
		m_peerDisconnected = false;
		return 1;
	}

	// Did connect return an unexpected error?
	if( errno != EINPROGRESS && errno != EWOULDBLOCK && errno != EINTR)
		throwError();

	// Wait for the connection to complete
	struct pollfd poll;
	poll.fd = m_socket;
	poll.events = POLLOUT;

	ret = TEMP_FAILURE_RETRY (::poll( &poll, 1, timeout));
	if( ret < 0) throwError();
	if( ret == 0) { // Timeout
		errno = ETIMEDOUT;
		return 0;
	}

	// Check for socket errors after poll success
	int sockError = 0;
	socklen_t errLen = sizeof(sockError);
	ret = ::getsockopt( m_socket, SOL_SOCKET, SO_ERROR, &sockError, &errLen);
	if( ret < 0) throwError();
	if( sockError != 0) { // Socket error
		errno = sockError;
		throwError();
	}

	m_peerDisconnected = false;
	return 1;
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

	if( ::getsockname( m_socket, (sockaddr*) &addr, &addr_len) < 0)
		throw SocketException("Fetch of local address failed (getsockname)");

	return inet_ntoa( addr.sin_addr);
}

unsigned short InternetSocket::getLocalPort() const
{
	sockaddr_in addr;
	socklen_t addr_len = sizeof(addr);

	if( ::getsockname( m_socket, (sockaddr*) &addr, &addr_len) < 0)
		throw SocketException("Fetch of local port failed (getsockname)");

	return ntohs( addr.sin_port);
}

std::string InternetSocket::getForeignAddress() const
{
	sockaddr_in addr;
	socklen_t addr_len = sizeof(addr);

	if( ::getpeername( m_socket, (sockaddr*) &addr, &addr_len) < 0)
		throw SocketException("Fetch of foreign address failed (getpeername)");

	return inet_ntoa( addr.sin_addr);
}

unsigned short InternetSocket::getForeignPort() const
{
	sockaddr_in addr;
	socklen_t addr_len = sizeof(addr);

	if( ::getpeername( m_socket, (sockaddr*) &addr, &addr_len) < 0)
		throw SocketException("Fetch of foreign port failed (getpeername)");

	return ntohs( addr.sin_port);
}

void InternetSocket::fillAddress( std::string_view address, unsigned short port, sockaddr_in& addr)
{
	const size_t len = address.length();

	// needed space is size plus null character
	if( len >= INET_ADDRSTRLEN)
		throw SocketException("IPv4 address is too long", false);

	addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	char buf[INET_ADDRSTRLEN];
	std::memcpy( buf, address.data(), len);
	buf[len] = 0;

	if( inet_aton( buf, &addr.sin_addr) == 0)
		throw SocketException("Invalid IPv4 address", false);
}
