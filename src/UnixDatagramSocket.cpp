#include "UnixDatagramSocket.h"
#include "SockAddress.h"
#include "Helper.h"

#include <sys/socket.h>

using namespace NET;

UnixDatagramSocket::UnixDatagramSocket()
: UnixSocket( SOCK_DGRAM, 0)
{}

UnixDatagramSocket UnixDatagramSocket::adoptNativeHandle( int sockfd)
{
	detail::validateSocket( sockfd, SOCK_DGRAM, 0);
	return UnixDatagramSocket( AdoptSocketTag{}, sockfd, SOCK_DGRAM);
}

std::pair<UnixDatagramSocket, UnixDatagramSocket> UnixDatagramSocket::createPair( bool nonBlocking)
{
	auto pair = detail::createUnixSocketPair( SOCK_DGRAM, nonBlocking);
	return {
		UnixDatagramSocket( AdoptSocketTag{}, pair.first,  SOCK_DGRAM),
		UnixDatagramSocket( AdoptSocketTag{}, pair.second, SOCK_DGRAM)
	};
}

IOResult UnixDatagramSocket::sendTo( const void* buffer, size_t len, std::string_view foreignPath)
{
	SockAddress addr( foreignPath);
	return SimpleSocket::sendTo( buffer, len, addr);
}

IOResult UnixDatagramSocket::receiveFrom( void* buffer, size_t len, std::string& sourcePath)
{
	SockAddress addr;
	IOResult res = SimpleSocket::receiveFrom( buffer, len, addr);
	if( res.status == IOStatus::Success)
	{
		sourcePath = addr.address();
	}
	return res;
}

IOResult UnixDatagramSocket::timedReceiveFrom( void* buffer, size_t len, std::string& sourcePath, int timeout)
{
	IOStatus res = SimpleSocket::waitReadable( timeout);
	if( res == IOStatus::Success)
		return receiveFrom( buffer, len, sourcePath);
	else
		return { res, 0 };
}
