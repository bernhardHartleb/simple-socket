#ifndef NET_PollHelper_h__
#define NET_PollHelper_h__

#include "TempFailure.h"
#include <poll.h>

namespace NET
{
	template <typename ReceiveFunc>
	int timedReceiveImpl( int fd, bool& peerDisconnected, int timeout, ReceiveFunc&& receiveFunc)
	{
		pollfd pfd {};
		pfd.fd = fd;
		pfd.events = POLLIN | POLLPRI | POLLRDHUP;

		const int ret = TEMP_FAILURE_RETRY(::poll( &pfd, 1, timeout));

		if( ret == 0) return 0;
		if( ret < 0) throw SocketException("timedReceive failed (poll)");

		if( pfd.revents & (POLLERR | POLLNVAL))
			throw SocketException("timedReceive failed (poll)");

		if( pfd.revents & (POLLRDHUP | POLLHUP))
			peerDisconnected = true;

		if( pfd.revents & (POLLIN | POLLPRI))
			return std::forward<ReceiveFunc>(receiveFunc)();

		return 0;
	}

} // namespace NET

#endif // NET_SockAddress_h__
