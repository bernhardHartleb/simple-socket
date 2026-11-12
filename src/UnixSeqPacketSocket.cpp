#include "UnixSeqPacketSocket.h"
#include "Helper.h"

#include <sys/socket.h>

using namespace NET;

UnixSeqPacketSocket::UnixSeqPacketSocket()
: UnixSocket( SOCK_SEQPACKET, 0)
{}

UnixSeqPacketSocket UnixSeqPacketSocket::adoptNativeHandle( int sockfd)
{
	detail::validateSocket( sockfd, SOCK_SEQPACKET, 0);
	return UnixSeqPacketSocket( AdoptSocketTag{}, sockfd, SOCK_SEQPACKET);
}

std::optional<UnixSeqPacketSocket> UnixSeqPacketSocket::accept() const
{
	int ret = SimpleSocket::accept();
	if( ret >= 0)
		return std::optional<UnixSeqPacketSocket>{ UnixSeqPacketSocket( AdoptSocketTag{}, ret, SOCK_SEQPACKET)};
	else
		return std::nullopt;
}

std::optional<UnixSeqPacketSocket> UnixSeqPacketSocket::timedAccept( int timeout) const
{
	int ret = SimpleSocket::timedAccept(timeout);
	if( ret >= 0)
		return std::optional<UnixSeqPacketSocket>{ UnixSeqPacketSocket( AdoptSocketTag{}, ret, SOCK_SEQPACKET)};
	else
		return std::nullopt;
}
