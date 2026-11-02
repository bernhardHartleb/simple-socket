#ifndef NET_PollHelper_h__
#define NET_PollHelper_h__

#include "SimpleSocket.h"
#include "TempFailure.h"

#include <sys/socket.h>
#include <poll.h>

namespace NET
{
	template <typename ReceiveFunc>
	IOResult timedReceiveImpl( int fd, bool connectionOriented, int timeout, ReceiveFunc&& receiveFunc)
	{
		const auto throwError = []() {
			throw SocketException("timedReceive failed");
		};

		pollfd pfd {};
		pfd.fd = fd;
		pfd.events = POLLIN | POLLRDHUP;

		const int ret = TEMP_FAILURE_RETRY(::poll( &pfd, 1, timeout));
		if( ret < 0)
			throwError();

		if( ret == 0) return { IOStatus::Timeout, 0 };

		if( pfd.revents & POLLNVAL)
			throw SocketException("timedReceive failed (invalid socket)", false);

		// Data has priority. There may be queued data followed by FIN
		if( pfd.revents & POLLIN)
			return std::forward<ReceiveFunc>(receiveFunc)();

		if( pfd.revents & (POLLRDHUP | POLLHUP))
			return { IOStatus::PeerDisconnected, 0 };

		if( pfd.revents & POLLERR) // async error with socket
		{
			int error = 0;
			socklen_t len = sizeof(error);

			if( ::getsockopt( fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
				throwError();

			auto status = SimpleSocket::classifyIoError( error, connectionOriented);
			if( status) return IOResult{ *status, 0 };

			throw SocketException("timedReceive failed", error);
		}

		throwError();
		// silence warning
		return { IOStatus::PeerDisconnected, 0 };
	}

} // namespace NET

#endif // NET_SockAddress_h__
