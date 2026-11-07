#ifndef NET_PollHelper_h__
#define NET_PollHelper_h__

#include "SimpleSocket.h"
#include "TempFailure.h"

#include <sys/socket.h>
#include <poll.h>
#include <utility>
#include <optional>
#include <chrono>

namespace NET::detail
{
	//! return correct IOStatus for each un-excpetional failures
	std::optional<IOStatus> classifyIoError( int error, bool connectionOriented) noexcept;

	//! maps errno values into user visible connect result
	std::optional<ConnectResult> connectResultFromError( int error) noexcept;

	//! returns true when to retry accept() due to a transient failure
	bool isTransientAcceptError( int error) noexcept;


	//! implements timedReceive() for all sockets
	template <typename ReceiveFunc>
	IOResult timedReceiveImpl( int fd, bool connectionOriented, int timeout, ReceiveFunc&& receiveFunc)
	{
		bool firstRun = true;
		using Clock = std::chrono::steady_clock;
		auto now = Clock::now();
		// negative timeout means wait forever
		const bool infinite = timeout < 0;
		const auto deadline = infinite ? Clock::time_point::max()
			: now + std::chrono::milliseconds(timeout);

		while(1)
		{
			// ensure now == deadline when timeout is zero
			if( !firstRun) now = Clock::now();
			// zero timeout returns only after first loop
			if( now > deadline && !firstRun)
				return { IOStatus::Timeout, 0 };

			auto remaining = std::chrono::ceil<std::chrono::milliseconds>(deadline - now);
			const int pollTimeout = static_cast<int>(remaining.count());

			pollfd pfd {};
			pfd.fd = fd;
			pfd.events = POLLIN | POLLRDHUP;

			int ret = ::poll( &pfd, 1, pollTimeout);
			if( ret == 0) return { IOStatus::Timeout, 0 };
			if( ret < 0)
			{
				if( errno == EINTR) continue;
				throw SocketException("timedReceive failed (poll)");
			}

			if( pfd.revents & POLLNVAL)
				throw SocketException("timedReceive failed (invalid socket)", false);

			// Data has priority. There may be queued data followed by FIN
			if( pfd.revents & POLLIN)
				return std::forward<ReceiveFunc>(receiveFunc)();

			if( pfd.revents & POLLERR) // async error with socket
			{
				int error = 0;
				socklen_t len = sizeof(error);
				if( ::getsockopt( fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
					throw SocketException("timedReceive failed (getsockopt)");

				// try again until timeout
				if( error == EAGAIN || error == EWOULDBLOCK)
					continue;

				auto status = classifyIoError( error, connectionOriented);
				if( status) return IOResult{ *status, 0 };

				throw SocketException("timedReceive failed", error);
			}

			if( connectionOriented && (pfd.revents & (POLLRDHUP | POLLHUP)))
				return { IOStatus::PeerDisconnected, 0 };

			throw SocketException("timedReceive failed", false);
		}
	}

} // namespace NET::detail

#endif // NET_PollHelper_h__
