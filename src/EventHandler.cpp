#include "EventHandler.h"

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include <algorithm>
#include <climits>
#include <cerrno>
#include <limits>
#include <utility>
#include <stdexcept>
#include <system_error>

using namespace NET;

namespace
{
	timespec toTimespec(std::chrono::milliseconds value)
	{
		const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(value);
		const auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(value - seconds);
		if (seconds.count() > std::numeric_limits<time_t>::max())
			throw std::out_of_range("EventHandler: timer duration is too large");

		return timespec{static_cast<time_t>(seconds.count()), static_cast<long>(nanoseconds.count())};
	}

	// Always restore the flag when an application callback throws
	struct AtomicFlagReset
	{
		std::atomic<bool>& flag;
		~AtomicFlagReset() { flag.store(false); }
	};
}

EventHandler::EventHandler(int batchSize)
: m_ready(static_cast<std::size_t>(batchSize))
{
	if (batchSize <= 0)
		throw std::invalid_argument("EventHandler: batchSize must be positive");

	m_epollFd = ::epoll_create1(EPOLL_CLOEXEC);
	if (m_epollFd < 0)
		throw std::system_error(errno, std::generic_category(), "epoll_create1");

	m_wakeFd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
	if (m_wakeFd < 0)
	{
		const int error = errno;
		::close(m_epollFd);
		throw std::system_error(error, std::generic_category(), "eventfd");
	}

	try {
		add(m_wakeFd, Read, [this](int, Events) { drainEvent(m_wakeFd); });
	} catch (...) {
		::close(m_wakeFd);
		::close(m_epollFd);
		throw;
	}
}

EventHandler::~EventHandler() noexcept
{
	// The caller must ensure run() has finished before destruction.
	for (const auto& entry : m_timers)
		::close(entry.second.fd);

	::close(m_wakeFd);
	::close(m_epollFd);
}

void EventHandler::add(int fd, Events interests, Callback callback)
{
	if (fd < 0 || !callback)
		throw std::invalid_argument("EventHandler: invalid fd or callback");

	if (m_byFd.count(fd))
		throw std::logic_error("EventHandler: descriptor already registered");

	if (m_nextToken == 0 || m_nextToken == std::numeric_limits<std::uint64_t>::max())
		throw std::overflow_error("EventHandler: registration IDs exhausted");

	const auto token = m_nextToken++;
	m_registrations.emplace(token, Registration{fd, std::move(callback)});
	try {
		m_byFd.emplace(fd, token);
	} catch (...) {
		m_registrations.erase(token);
		throw;
	}

	epoll_event event{};
	event.events = interests;
	event.data.u64 = token; // Not fd: stale notifications cannot target a reused fd.
	if (::epoll_ctl(m_epollFd, EPOLL_CTL_ADD, fd, &event) != 0)
	{
		const int error = errno;
		m_byFd.erase(fd);
		m_registrations.erase(token);
		throw std::system_error(error, std::generic_category(), "epoll_ctl ADD");
	}
}

void EventHandler::modify(int fd, Events interests)
{
	const auto it = m_byFd.find(fd);
	if (it == m_byFd.end())
		throw std::out_of_range("EventHandler: descriptor not registered");

	const auto token = it->second;
	epoll_event event{};
	event.events = interests;
	event.data.u64 = token;
	if (::epoll_ctl(m_epollFd, EPOLL_CTL_MOD, fd, &event) != 0)
		throw std::system_error(errno, std::generic_category(), "epoll_ctl MOD");
}

void EventHandler::remove(int fd)
{
	auto it = m_byFd.find(fd);
	if (it == m_byFd.end()) return;

	if (::epoll_ctl(m_epollFd, EPOLL_CTL_DEL, fd, nullptr) != 0)
	{
		// Already closed/unregistered is harmless; remove local bookkeeping.
		if (errno != EBADF && errno != ENOENT)
			throw std::system_error(errno, std::generic_category(), "epoll_ctl DEL");
	}
	m_registrations.erase(it->second);
	m_byFd.erase(it);
}

EventHandler::TimerId EventHandler::addOneShotTimer(std::chrono::milliseconds delay, TimerCallback callback)
{
	return addTimer(delay, std::chrono::milliseconds(0), std::move(callback));
}

EventHandler::TimerId EventHandler::addPeriodicTimer(std::chrono::milliseconds interval, TimerCallback callback)
{
	return addTimer(interval, interval, std::move(callback));
}

EventHandler::TimerId EventHandler::addTimer(
	std::chrono::milliseconds delay,
	std::chrono::milliseconds interval,
	TimerCallback callback)
{
	if (delay.count() <= 0 || interval.count() < 0 || !callback)
		throw std::invalid_argument("EventHandler: invalid timer duration or callback");

	if (m_nextTimerId == 0 || m_nextTimerId == std::numeric_limits<TimerId>::max())
		throw std::overflow_error("EventHandler: timer IDs exhausted");

	itimerspec config{};
	config.it_value = toTimespec(delay);
	config.it_interval = toTimespec(interval);
	const int fd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
	if (fd < 0)
		throw std::system_error(errno, std::generic_category(), "timerfd_create");

	if (::timerfd_settime(fd, 0, &config, nullptr) < 0)
	{
		const int error = errno;
		::close(fd);
		throw std::system_error(error, std::generic_category(), "timerfd_settime");
	}

	const TimerId id = m_nextTimerId++;
	try {
		m_timers.emplace(id, Timer{fd, std::move(callback), delay, interval, 0});
		try {
			add(fd, Read, [this, id](int, Events) { dispatchTimer(id); });
		} catch (...) {
			m_timers.erase(id);
			throw;
		}
	} catch (...) {
		::close(fd);
		throw;
	}
	return id;
}

void EventHandler::restartTimer(TimerId id)
{
	const auto it = m_timers.find(id);
	if (it == m_timers.end())
		throw std::out_of_range("EventHandler: unknown timer ID");

	restartTimer(id, it->second.delay);
}

void EventHandler::restartTimer(TimerId id, std::chrono::milliseconds delay)
{
	if (delay.count() <= 0)
		throw std::invalid_argument("EventHandler: delay must be positive");

	const auto it = m_timers.find(id);
	if (it == m_timers.end())
		throw std::out_of_range("EventHandler: unknown timer ID");

	if (it->second.version == std::numeric_limits<std::uint64_t>::max())
		throw std::overflow_error("EventHandler: timer version exhausted");

	itimerspec config{};
	config.it_value = toTimespec(delay);
	config.it_interval = toTimespec(it->second.interval);
	if (::timerfd_settime(it->second.fd, 0, &config, nullptr) < 0)
		throw std::system_error(errno, std::generic_category(), "timerfd_settime");

	it->second.delay = delay;
	++it->second.version;
}

void EventHandler::removeTimer(TimerId id)
{
	const auto it = m_timers.find(id);
	if (it == m_timers.end()) return;
	const int fd = it->second.fd;
	remove(fd);
	::close(fd);
	m_timers.erase(it);
}

void EventHandler::dispatchTimer(TimerId id)
{
	const auto it = m_timers.find(id);
	if (it == m_timers.end()) return;

	std::uint64_t expirations = drainEvent(it->second.fd);
	if (expirations <= 0) return;

	const bool oneShot = (it->second.interval.count() == 0);
	const auto version = it->second.version;
	// Take ownership of the actual callback during execution
	auto callback = std::move(it->second.callback);

	// A one-shot stays registered through its callback so it can restart itself
	auto finish = [&]() {
		auto current = m_timers.find(id);
		if (current == m_timers.end()) return;

		if (oneShot && current->second.version == version) {
			removeTimer(id);
		} else {
			current->second.callback = std::move(callback);
		}
	};

	// Handle a callback removing itself or throwing
	try {
		callback(expirations);
	} catch(...) {
		finish();
		throw;
	}
	finish();
}

void EventHandler::defer(std::function<void()> callback)
{
	if (!callback)
		throw std::invalid_argument("EventHandler: empty defer callback");

	m_deferred.push_back(std::move(callback));
	signalEvent(m_wakeFd);
}

void EventHandler::dispatchDeferred()
{
	// A callback scheduled by a deferred callback belongs to the *next* cycle.
	const auto count = m_deferred.size();
	for (std::size_t i = 0; i < count && !m_stopRequested.load(); ++i)
	{
		auto callback = std::move(m_deferred.front());
		m_deferred.pop_front();
		try {
			callback();
		} catch (...) {
			// The popped callback failed; preserve wakeup for any remaining work.
			if (!m_deferred.empty()) signalEvent(m_wakeFd);
			throw;
		}
	}
}

std::uint64_t EventHandler::drainEvent(int fd)
{
	std::uint64_t value = 0;
	while(1)
	{
		const ssize_t count = ::read(fd, &value, sizeof(value));
		if (count == static_cast<ssize_t>(sizeof(value))) return value;
		if (count < 0 && errno == EINTR) continue;
		if (count < 0 && errno == EAGAIN) return 0;
		if (count < 0)
			throw std::system_error(errno, std::generic_category(), "eventfd read");
		throw std::runtime_error("EventHandler: short eventfd read");
	}
}

void EventHandler::signalEvent(int fd) noexcept
{
	const std::uint64_t value = 1;
	while(1)
	{
		const ssize_t count = ::write(fd, &value, sizeof(value));
		if (count == static_cast<ssize_t>(sizeof(value))) return;
		if (count < 0 && errno == EINTR) continue;
		// EAGAIN means already signaled (counter saturated)
		return;
	}
}

void EventHandler::run()
{
	// Do not clear stop signal, a stop requested before startup is sticky.
	// Only one thread may dispatch events, and nested run()/wait() is unsupported.
	while (!m_stopRequested.load()) wait(-1);
}

void EventHandler::stop() noexcept
{
	m_stopRequested.store(true);
	signalEvent(m_wakeFd);
}

void EventHandler::resetStop()
{
	if (m_waiting.load())
		throw std::logic_error("EventHandler: event loop is active");

	// Caller must ensure no concurrent stop() request during this explicit reset.
	m_stopRequested.store(false);
	drainEvent(m_wakeFd);
	if (!m_deferred.empty()) signalEvent(m_wakeFd);
}

int EventHandler::wait(int timeoutMs)
{
	if (m_waiting.exchange(true))
		throw std::logic_error("EventHandler: nested/concurrent wait");

	const AtomicFlagReset reset{m_waiting};
	if (m_stopRequested.load()) return 0;

	using Clock = std::chrono::steady_clock;
	const bool infinite = timeoutMs < 0;
	const auto deadline = infinite
		? Clock::time_point{}
		: Clock::now() + std::chrono::milliseconds(timeoutMs);

	int count = 0;
	while(1)
	{
		int waitMs = -1;
		if (!infinite)
		{
			const auto now = Clock::now();
			const auto remaining = std::max(deadline - now, Clock::duration::zero());
			const auto ms = std::chrono::ceil<std::chrono::milliseconds>(remaining).count();
			waitMs = static_cast<int>(std::min<decltype(ms)>(ms, INT_MAX));
		}
		count = ::epoll_wait(m_epollFd, m_ready.data(), static_cast<int>(m_ready.size()), waitMs);
		if (count < 0)
		{
			if (errno == EINTR) continue;
			throw std::system_error(errno, std::generic_category(), "epoll_wait");
		}
		break;
	}

	for (unsigned i = 0; i < static_cast<unsigned>(count); ++i)
	{
		const auto token = m_ready[i].data.u64;
		const auto it = m_registrations.find(token);
		if (it == m_registrations.end()) continue;

		const int fd = it->second.fd;
		const Events events = m_ready[i].events;
		// Take ownership of the actual callback during execution
		auto callback = std::move(it->second.callback);

		// Restore if the registration still exists
		auto restore = [&]() {
			auto current = m_registrations.find(token);
			if (current != m_registrations.end() && !current->second.callback) {
				current->second.callback = std::move(callback);
			}
		};

		// Handle a callback removing itself or throwing
		try {
			callback(fd, events);
		} catch(...) {
			restore();
			throw;
		}
		restore();

		if (m_stopRequested.load()) break;
	}

	if (!m_stopRequested.load() && !m_deferred.empty())
		dispatchDeferred();

	return count;
}
