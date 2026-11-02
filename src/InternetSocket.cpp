#include "InternetSocket.h"
#include "SockAddress.h"
#include "TempFailure.h"
#include "PollHelper.h"

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

void InternetSocket::connect( std::string_view foreignAddress, unsigned short foreignPort)
{
	SockAddress addr( m_family, foreignAddress, foreignPort);

	if( ::connect( m_socket, addr.data(), addr.length()) < 0)
		throw SocketException("Connect failed (connect)");
}

ConnectResult InternetSocket::timedConnect( std::string_view foreignAddress, unsigned short foreignPort, int timeout)
{
	SockAddress addr( m_family, foreignAddress, foreignPort);

	// Set O_NONBLOCK and restore blocking state on destruction
	NonBlockingGuard nonBlocking( m_socket);

	const auto throwError = []() {
		throw SocketException("timedConnect failed");
	};

	// Asynchronous connection attempt
	int ret = ::connect( m_socket, addr.data(), addr.length());
	if( ret == 0) // Already connected
		return ConnectResult::Connected;

	// known connection issue?
	auto result = detail::connectResultFromError(errno);
	if( result) return *result;

	// should we continue with poll?
	switch(errno)
	{
		case EINPROGRESS:
		case EWOULDBLOCK:
		case EINTR:
			break;
		default:
			throwError();
	}

	// Wait for the connection to complete
	pollfd pfd {};
	pfd.fd = m_socket;
	pfd.events = POLLOUT;

	ret = TEMP_FAILURE_RETRY (::poll( &pfd, 1, timeout));
	if( ret < 0) throwError();

	if( ret == 0) return ConnectResult::Timeout;

	// Check for socket errors after poll success
	int error = 0;
	socklen_t len = sizeof(error);
	if( ::getsockopt( m_socket, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
		throwError();

	if( error == 0) return ConnectResult::Connected;

	// known connection issues should not throw
	result = detail::connectResultFromError(error);
	if( result) return *result;

	throw SocketException("timedConnect failed", error);
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
