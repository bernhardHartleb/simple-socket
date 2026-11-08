#ifndef NET_PollWait_h__
#define NET_PollWait_h__

#include <poll.h>
#include <climits>
#include <functional>
#include <optional>
#include <chrono>

namespace NET::detail
{
	//! polled waiting for socket event until deadline
	template <typename Callback>
	auto pollWait( int fd, short events, int timeout, Callback&& callback)
	{
		// Callback must return std::optional<T>
		using Result = std::invoke_result_t<Callback&, short>;
		using Clock = std::chrono::steady_clock;
		const bool infinite = timeout < 0;
		const auto deadline = infinite
			? Clock::time_point{}
			: Clock::now() + std::chrono::milliseconds(timeout);

		while(1)
		{
			int wait_ms = -1;
			if( !infinite)
			{
				const auto remaining = std::max( deadline - Clock::now(), Clock::duration::zero());
				const auto ms = std::chrono::ceil<std::chrono::milliseconds>( remaining).count();
				wait_ms = static_cast<int>( std::min<decltype(ms)>( ms, INT_MAX));
			}
			pollfd pfd {fd, events, 0};
			const int ret = ::poll( &pfd, 1, wait_ms);

			if( ret < 0)
			{
				if( errno == EINTR) continue;
				throw SocketException("pollWait failed (poll)");
			}
			else if( ret > 0)
			{
				if( pfd.revents & POLLNVAL)
					throw SocketException("pollWait failed (invalid socket)", false);

				if( auto result = std::invoke( callback, pfd.revents))
					return result;
			}

			if( Clock::now() >= deadline)
				return Result {}; // Timeout
		}
	}

} // namespace NET::detail

#endif // NET_PollWait_h__
