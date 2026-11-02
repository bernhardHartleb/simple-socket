#include "UnixDatagramSocket.h"
#include "SockAddress.h"
#include "TempFailure.h"
#include "PollHelper.h"

#include <sys/socket.h>

using namespace NET;

UnixDatagramSocket::UnixDatagramSocket()
: UnixSocket( SOCK_DGRAM, 0)
{}

UnixDatagramSocket UnixDatagramSocket::adoptNativeHandle( int sockfd)
{
	validateSocket( sockfd, SOCK_DGRAM, 0);
	return UnixDatagramSocket( AdoptSocketTag{}, sockfd, SOCK_DGRAM);
}

IOResult UnixDatagramSocket::sendTo( const void* buffer, size_t len, std::string_view foreignPath)
{
	SockAddress addr( foreignPath);

	ssize_t ret = TEMP_FAILURE_RETRY (::sendto( m_socket, (const raw_type*)buffer, len, 0, addr.data(), addr.length()));
	if( ret >= 0)
		return { IOStatus::Success, static_cast<size_t>(ret) };

	auto status = detail::classifyIoError( errno, m_connectionOriented);
	if( status) return IOResult{ *status, 0 };

	throw SocketException("Send failed (sendto)");
}

IOResult UnixDatagramSocket::receiveFrom( void* buffer, size_t len, std::string& sourcePath)
{
	SockAddress addr;

	ssize_t ret = TEMP_FAILURE_RETRY (::recvfrom( m_socket, (raw_type*)buffer, len, 0, addr.data(), addr.lengthPtr()));
	if( ret < 0)
	{
		auto status = detail::classifyIoError( errno, m_connectionOriented);
		if( status) return IOResult{ *status, 0 };

		throw SocketException("Receive failed (recvfrom)");
	}
	sourcePath = addr.address();
	return { IOStatus::Success, static_cast<size_t>(ret) };
}

IOResult UnixDatagramSocket::timedReceiveFrom( void* buffer, size_t len, std::string& sourcePath, int timeout)
{
	return detail::timedReceiveImpl( m_socket, m_connectionOriented, timeout,
		[&] { return receiveFrom( buffer, len, sourcePath); });
}
