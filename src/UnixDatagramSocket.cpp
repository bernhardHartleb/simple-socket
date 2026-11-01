#include "UnixDatagramSocket.h"
#include "SockAddress.h"
#include "TempFailure.h"
#include "PollHelper.h"

#include <sys/socket.h>

using namespace NET;

UnixDatagramSocket::UnixDatagramSocket()
: UnixSocket( SOCK_DGRAM, 0)
{}

IOResult UnixDatagramSocket::sendTo( const void* buffer, size_t len, std::string_view foreignPath)
{
	SockAddress addr( foreignPath);

	ssize_t ret = TEMP_FAILURE_RETRY (::sendto( m_socket, (const raw_type*)buffer, len, 0, addr.data(), addr.length()));

	// Write out the whole buffer as a single message
	if( ret >= 0 && static_cast<size_t>(ret) == len)
		return { IOStatus::Success, static_cast<size_t>(ret) };

	const int error = errno;
	if( isNonFatalDatagramError(error))
	{
		return { IOStatus::DestinationUnavailable, 0 };
	}
	throw SocketException("Send failed (sendto)");

}

IOResult UnixDatagramSocket::receiveFrom( void* buffer, size_t len, std::string& sourcePath)
{
	SockAddress addr;

	ssize_t ret = TEMP_FAILURE_RETRY (::recvfrom( m_socket, (raw_type*)buffer, len, 0, addr.data(), addr.lengthPtr()));
	if( ret < 0)
		throw SocketException("Receive failed (recvfrom)");

	sourcePath = addr.address();
	return { IOStatus::Success, static_cast<size_t>(ret) };
}

IOResult UnixDatagramSocket::timedReceiveFrom( void* buffer, size_t len, std::string& sourcePath, int timeout)
{
	return timedReceiveImpl( m_socket, m_peerDisconnected, timeout,
		[&] { return receiveFrom( buffer, len, sourcePath); });
}
