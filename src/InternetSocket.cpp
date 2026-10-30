#include "InternetSocket.h"
#include "SockAddress.h"
#include "TempFailure.h"

#include <sys/socket.h>
#include <poll.h>
#include <fcntl.h>

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

InternetSocket::InternetSocket( AdoptSocketTag, int sockfd)
: SimpleSocket( sockfd)
, m_family( socketAddressFamily(sockfd))
{}

void InternetSocket::setReuseAddress( bool enable)
{
	const int value = enable;
	if( ::setsockopt( m_socket, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)) < 0)
		throw SocketException("Failed to set address reuse (setsockopt)");
}

void InternetSocket::connect( std::string_view foreignAddress, unsigned short foreignPort)
{
	SockAddress addr( m_family, foreignAddress, foreignPort);

	if( ::connect( m_socket, addr.data(), addr.length()) < 0)
		throw SocketException("Connect failed (connect)");

	m_peerDisconnected = false;
}

int InternetSocket::timedConnect( std::string_view foreignAddress, unsigned short foreignPort, int timeout)
{
	SockAddress addr( m_family, foreignAddress, foreignPort);

	// Set O_NONBLOCK and restore blocking state on destruction
	NonBlockingGuard nonBlocking( m_socket);

	const auto throwError = []() {
		throw SocketException("timedConnect failed (connect)");
	};

	// Asynchronous connection attempt
	int ret = ::connect( m_socket, addr.data(), addr.length());
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
		throw SocketException("Fetch of foreign address failed (getpeername)");

	return addr.address();
}

unsigned short InternetSocket::getForeignPort() const
{
	SockAddress addr;

	if( ::getpeername( m_socket, addr.data(), addr.lengthPtr()) < 0)
		throw SocketException("Fetch of foreign port failed (getpeername)");

	return addr.port();
}
