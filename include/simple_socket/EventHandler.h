#ifndef NET_EventHandler_h__
#define NET_EventHandler_h__

#include <sys/epoll.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <vector>
#include <deque>

namespace NET
{
	// Single-threaded Linux epoll dispatcher.
	// Owns only its internal epoll/eventfd and timerfd handles.
	// Only stop() is cross-thread safe.
	class EventHandler final
	{
	public:
		using Events = std::uint32_t;
		using Callback = std::function<void(int, Events)>;
		using TimerId = std::uint64_t;
		// For periodic timers, expirations can be >1 when the event loop falls behind.
		using TimerCallback = std::function<void(std::uint64_t expirations)>;

		enum Event : std::uint32_t
		{
			Read       = EPOLLIN,
			Write      = EPOLLOUT,
			Priority   = EPOLLPRI,
			PeerClosed = EPOLLRDHUP,
			Error      = EPOLLERR,
			Hangup     = EPOLLHUP
		};

		explicit EventHandler(int batchSize = 64);
		~EventHandler() noexcept;

		EventHandler(const EventHandler&) = delete;
		EventHandler& operator=(const EventHandler&) = delete;
		EventHandler(EventHandler&&) = delete;
		EventHandler& operator=(EventHandler&&) = delete;

		// The watched fd must remain open until remove(fd). Only the caller owns it.
		// Call these from the event-loop thread, including from callbacks.
		void add(int fd, Events interests, Callback callback);
		void modify(int fd, Events interests);
		void remove(int fd); // Idempotent, including inside callbacks.

		// Owned timers (CLOCK_MONOTONIC); delay/interval must be positive.
		// One-shot timers are removed automatically immediately before their callback.
		// Timer IDs are not file descriptors. Timer methods are event-loop-thread only.
		TimerId addOneShotTimer(std::chrono::milliseconds delay, TimerCallback callback);
		TimerId addPeriodicTimer(std::chrono::milliseconds interval, TimerCallback callback);

		void restartTimer(TimerId id); // Reuse the most recently configured delay.
		void restartTimer(TimerId id, std::chrono::milliseconds delay);
		void removeTimer(TimerId id); // Idempotent, including inside callbacks.

		// Queue an idle-phase callback (FIFO). Single-threaded, including callbacks.
		// Deferred callbacks run after ready I/O/timers, not recursively. Callbacks
		// deferred by another deferred callback run in a later wait() cycle.
		// An eventfd write wakes epoll even if no external fd is ready.
		void defer(std::function<void()> callback);

		// Returns number of kernel notifications. A zero timeout always attempts epoll once.
		// Negative timeout means wait indefinitely. EINTR does not extend the deadline.
		int wait(int timeoutMs = -1);

		// Keeps waiting for new events until stopped
		void run();

		// May be called from another thread to interrupt run()/wait(-1).
		// Does not make registration or timer methods thread-safe.
		void stop() noexcept;

		// Explicitly allow a later run(). Call only after prior run()/wait() finishes,
		// with no concurrent stop() request. All dispatch operations, including
		// run(), wait(), and resetStop(), must remain on one thread; do not nest them.
		void resetStop();

	private:
		struct Registration { int fd; Callback callback; };
		struct Timer {
			int fd;
			TimerCallback callback;
			std::chrono::milliseconds delay;
			std::chrono::milliseconds interval;
			std::uint64_t version = 0;
		};

		TimerId addTimer(std::chrono::milliseconds delay,
		                 std::chrono::milliseconds interval,
		                 TimerCallback callback);
		void dispatchTimer(TimerId id);
		void dispatchDeferred();

		std::uint64_t drainEvent(int fd);
		void signalEvent(int fd) noexcept;

		int m_epollFd = -1;
		int m_wakeFd = -1;
		std::vector<epoll_event> m_ready;
		std::uint64_t m_nextToken = 1;
		TimerId m_nextTimerId = 1;

		std::unordered_map<std::uint64_t, Registration> m_registrations;
		std::unordered_map<int, std::uint64_t> m_byFd;
		std::unordered_map<TimerId, Timer> m_timers;
		std::deque<std::function<void()>> m_deferred;

		std::atomic<bool> m_stopRequested{false};
		std::atomic<bool> m_waiting{false};
	};

} // namespace NET

#endif // NET_EventHandler_h__
