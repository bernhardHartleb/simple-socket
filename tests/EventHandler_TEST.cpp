#include <cppunit/extensions/HelperMacros.h>
#include <simple_socket/EventHandler.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <thread>
#include <unistd.h>
#include <fcntl.h>

using namespace std::chrono_literals;

class EventHandler_TEST : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(EventHandler_TEST);
	CPPUNIT_TEST(testDescriptorCallbacks);
	CPPUNIT_TEST(testOneShotTimerRearm);
	CPPUNIT_TEST(testPeriodicTimerSelfCancellation);
	CPPUNIT_TEST(testCancelPendingTimer);
	CPPUNIT_TEST(testPeriodicTimerExpirationCount);
	CPPUNIT_TEST(testStopWakesRunThread);
	CPPUNIT_TEST(testCallbackTriggeredStop);
	CPPUNIT_TEST(testInvalidTimerRequests);
	CPPUNIT_TEST(testRemoveOtherDescriptors);
	CPPUNIT_TEST(testDeferredFifoAndNextCycle);
	CPPUNIT_TEST(testDeferredExceptionLeavesRemainingWork);
	CPPUNIT_TEST(testOneShotCanRearmItself);
	CPPUNIT_TEST(testRestartPendingOneShotChangesDeadline);
	CPPUNIT_TEST(testPeriodicTimerRestartPreservesInterval);
	CPPUNIT_TEST(testThrowingOneShotGetsCleanedUp);
	CPPUNIT_TEST(testInvalidTimerAndDeferArguments);
	CPPUNIT_TEST(testNestedWaitIsRejected);
	CPPUNIT_TEST(testStopBeforeRunIsSticky);
	CPPUNIT_TEST(testDeferredStopPreservesRemainingWork);
	CPPUNIT_TEST(testStopDuringRunStartupNotLost);
	CPPUNIT_TEST_SUITE_END();

public:
	// Existing zero-timeout behavior and descriptor callbacks.
	void testDescriptorCallbacks()
	{
		NET::EventHandler ev(1);
		CPPUNIT_ASSERT_EQUAL(0, ev.wait(0));

		int fds[2];
		CPPUNIT_ASSERT_EQUAL(0, ::pipe2(fds, O_NONBLOCK | O_CLOEXEC));

		int calls = 0;
		ev.add(fds[0], NET::EventHandler::Read, [&](int fd, std::uint32_t) {
			char ch;
			CPPUNIT_ASSERT_EQUAL(static_cast<ssize_t>(1), ::read(fd, &ch, 1));
			CPPUNIT_ASSERT_EQUAL('x', ch);
			++calls;
			ev.remove(fd);
		});

		const char ch = 'x';
		CPPUNIT_ASSERT_EQUAL(static_cast<ssize_t>(1), ::write(fds[1], &ch, 1));
		CPPUNIT_ASSERT(ev.wait(0) > 0);
		CPPUNIT_ASSERT_EQUAL(1, calls);
		CPPUNIT_ASSERT_EQUAL(0, ev.wait(0));

		::close(fds[0]);
		::close(fds[1]);
	}

	// One-shot unregistration and rearming inside callback.
	void testOneShotTimerRearm()
	{
		NET::EventHandler ev;
		int calls = 0;
		NET::EventHandler::TimerId first = 0;
		first = ev.addOneShotTimer(10ms, [&](std::uint64_t n) {
			CPPUNIT_ASSERT_EQUAL(std::uint64_t{1}, n);
			++calls;
			ev.removeTimer(first);
			ev.addOneShotTimer(10ms, [&](std::uint64_t m) {
				CPPUNIT_ASSERT_EQUAL(std::uint64_t{1}, m);
				++calls;
			});
		});

		CPPUNIT_ASSERT(ev.wait(200) >= 1);
		CPPUNIT_ASSERT(ev.wait(200) >= 1);
		CPPUNIT_ASSERT_EQUAL(2, calls);
		CPPUNIT_ASSERT_EQUAL(0, ev.wait(0));
	}

	// Periodic callbacks can safely cancel themselves.
	void testPeriodicTimerSelfCancellation()
	{
		NET::EventHandler ev;
		NET::EventHandler::TimerId recurring = 0;
		int hits = 0;
		recurring = ev.addPeriodicTimer(10ms, [&](std::uint64_t n) {
			CPPUNIT_ASSERT(n >= 1);
			++hits;
			if (hits == 3)
				ev.removeTimer(recurring);
		});

		for (int i = 0; i < 10 && hits < 3; ++i)
			ev.wait(100);
		CPPUNIT_ASSERT_EQUAL(3, hits);
		CPPUNIT_ASSERT_EQUAL(0, ev.wait(40));
	}

	// Pending timer can be cancelled without invocation.
	void testCancelPendingTimer()
	{
		NET::EventHandler ev;
		int calls = 0;
		auto id = ev.addOneShotTimer(10ms, [&](std::uint64_t) { ++calls; });
		ev.removeTimer(id);
		ev.removeTimer(id);

		CPPUNIT_ASSERT_EQUAL(0, ev.wait(40));
		CPPUNIT_ASSERT_EQUAL(0, calls);
	}

	// Time accumulated while not polling is reflected in periodic expiration count.
	void testPeriodicTimerExpirationCount()
	{
		NET::EventHandler ev;
		std::uint64_t observed = 0;
		auto id = ev.addPeriodicTimer(5ms, [&](std::uint64_t n) { observed = n; });
		std::this_thread::sleep_for(55ms);

		CPPUNIT_ASSERT(ev.wait(0) > 0);
		CPPUNIT_ASSERT(observed >= 2);
		ev.removeTimer(id);
	}

	// An internal eventfd lets stop() wake another thread waiting in epoll_wait(-1).
	void testStopWakesRunThread()
	{
		NET::EventHandler ev;
		std::atomic<bool> entered{false};
		std::thread worker([&] {
			entered.store(true);
			ev.run();
		});

		while (!entered.load())
			std::this_thread::yield();
		std::this_thread::sleep_for(30ms);

		const auto before = std::chrono::steady_clock::now();
		ev.stop();
		worker.join();
		CPPUNIT_ASSERT(std::chrono::steady_clock::now() - before < 500ms);
	}

	// Callback-triggered stop also works and prevents further callbacks in batch.
	void testCallbackTriggeredStop()
	{
		NET::EventHandler ev;
		int callbacks = 0;
		ev.addOneShotTimer(10ms, [&](std::uint64_t) {
			++callbacks;
			ev.stop();
		});

		ev.run();
		CPPUNIT_ASSERT_EQUAL(1, callbacks);
	}

	// Invalid timer requests are rejected.
	void testInvalidTimerRequests()
	{
		NET::EventHandler ev;
		CPPUNIT_ASSERT_THROW(ev.addOneShotTimer(0ms, [](std::uint64_t) {}), std::invalid_argument);
		CPPUNIT_ASSERT_THROW(ev.addPeriodicTimer(-1ms, [](std::uint64_t) {}), std::invalid_argument);
	}

	// A callback can remove another descriptor already reported
	// in the same epoll batch.
	void testRemoveOtherDescriptors()
	{
		int p1[2], p2[2];
		CPPUNIT_ASSERT_EQUAL(0, ::pipe2(p1, O_NONBLOCK | O_CLOEXEC));
		CPPUNIT_ASSERT_EQUAL(0, ::pipe2(p2, O_NONBLOCK | O_CLOEXEC));

		NET::EventHandler ev;
		int called = 0;

		ev.add(p1[0], NET::EventHandler::Read,
			[&](int, std::uint32_t) {
				++called;
				ev.remove(p2[0]);
				ev.remove(p1[0]);
			});

		ev.add(p2[0], NET::EventHandler::Read,
			[&](int, std::uint32_t) {
				++called;
				ev.remove(p1[0]);
				ev.remove(p2[0]);
			});

		CPPUNIT_ASSERT_EQUAL(static_cast<ssize_t>(1), ::write(p1[1], "A", 1));
		CPPUNIT_ASSERT_EQUAL(static_cast<ssize_t>(1), ::write(p2[1], "B", 1));

		const auto n = ev.wait(0);

		CPPUNIT_ASSERT_EQUAL(2, n);
		CPPUNIT_ASSERT_EQUAL(1, called);
		CPPUNIT_ASSERT_EQUAL(0, ev.wait(0));

		for (int fd : p1) ::close(fd);
		for (int fd : p2) ::close(fd);
	}

	void testDeferredFifoAndNextCycle()
	{
		NET::EventHandler ev;
		std::vector<int> order;

		ev.defer([&] {
			order.push_back(1);
			ev.defer([&] { order.push_back(3); });
		});
		ev.defer([&] { order.push_back(2); });

		CPPUNIT_ASSERT(order.empty());
		CPPUNIT_ASSERT(ev.wait(200) > 0);
		CPPUNIT_ASSERT((order == std::vector<int>{1, 2}));
		CPPUNIT_ASSERT(ev.wait(200) > 0);
		CPPUNIT_ASSERT((order == std::vector<int>{1, 2, 3}));
		CPPUNIT_ASSERT_EQUAL(0, ev.wait(0));
	}

	void testDeferredExceptionLeavesRemainingWork()
	{
		NET::EventHandler ev;
		int calls = 0;

		ev.defer([&] { ++calls; throw std::runtime_error("intentional"); });
		ev.defer([&] { ++calls; });

		CPPUNIT_ASSERT_THROW(ev.wait(0), std::runtime_error);
		CPPUNIT_ASSERT_EQUAL(1, calls);
		CPPUNIT_ASSERT(ev.wait(0) > 0);
		CPPUNIT_ASSERT_EQUAL(2, calls);
	}

	void testOneShotCanRearmItself()
	{
		NET::EventHandler ev;
		int hits = 0;
		NET::EventHandler::TimerId id = 0;

		id = ev.addOneShotTimer(3ms, [&](std::uint64_t count) {
			CPPUNIT_ASSERT(count >= 1);
			if (++hits < 4) ev.restartTimer(id, 3ms);
		});

		while (hits < 4) CPPUNIT_ASSERT(ev.wait(200) > 0);
		CPPUNIT_ASSERT_EQUAL(4, hits);
		CPPUNIT_ASSERT_THROW(ev.restartTimer(id), std::out_of_range);
		CPPUNIT_ASSERT_EQUAL(0, ev.wait(0));
	}

	void testRestartPendingOneShotChangesDeadline()
	{
		NET::EventHandler ev;
		int calls = 0;
		const auto id = ev.addOneShotTimer(80ms, [&](std::uint64_t) { ++calls; });

		std::this_thread::sleep_for(10ms);
		ev.restartTimer(id, 80ms);
		CPPUNIT_ASSERT_EQUAL(0, ev.wait(0));
		CPPUNIT_ASSERT(ev.wait(300) > 0);
		CPPUNIT_ASSERT_EQUAL(1, calls);
	}

	void testPeriodicTimerRestartPreservesInterval()
	{
		NET::EventHandler ev;
		int calls = 0;
		const auto id = ev.addPeriodicTimer(3ms, [&](std::uint64_t) { ++calls; });

		CPPUNIT_ASSERT(ev.wait(200) > 0);
		CPPUNIT_ASSERT_EQUAL(1, calls);
		ev.restartTimer(id, 60ms);
		ev.restartTimer(id);
		CPPUNIT_ASSERT_EQUAL(0, ev.wait(0));
		CPPUNIT_ASSERT(ev.wait(200) > 0);
		CPPUNIT_ASSERT_EQUAL(2, calls);
		CPPUNIT_ASSERT(ev.wait(200) > 0);
		CPPUNIT_ASSERT_EQUAL(3, calls);
		ev.removeTimer(id);
	}

	void testThrowingOneShotGetsCleanedUp()
	{
		NET::EventHandler ev;
		int calls = 0;
		const auto id = ev.addOneShotTimer(2ms, [&](std::uint64_t) {
			++calls;
			throw std::runtime_error("timer failure");
		});

		CPPUNIT_ASSERT_THROW(ev.wait(200), std::runtime_error);
		CPPUNIT_ASSERT_EQUAL(1, calls);
		CPPUNIT_ASSERT_THROW(ev.restartTimer(id), std::out_of_range);
		CPPUNIT_ASSERT_EQUAL(0, ev.wait(0));
	}

	void testInvalidTimerAndDeferArguments()
	{
		NET::EventHandler ev;
		CPPUNIT_ASSERT_THROW(ev.restartTimer(999), std::out_of_range);
		const auto id = ev.addOneShotTimer(30ms, [](std::uint64_t) {});
		CPPUNIT_ASSERT_THROW(ev.restartTimer(id, 0ms), std::invalid_argument);
		ev.removeTimer(id);
		CPPUNIT_ASSERT_THROW(ev.defer({}), std::invalid_argument);
	}

	void testNestedWaitIsRejected()
	{
		NET::EventHandler ev;
		bool rejected = false;
		ev.defer([&] {
			CPPUNIT_ASSERT_THROW(ev.wait(0), std::logic_error);
			rejected = true;
		});
		ev.wait(0);
		CPPUNIT_ASSERT(rejected);
	}

	void testStopBeforeRunIsSticky()
	{
		NET::EventHandler ev;
		ev.stop();
		ev.run();  // Must return immediately.
		ev.resetStop();

		int calls = 0;
		ev.addOneShotTimer(2ms, [&](std::uint64_t) { ++calls; ev.stop(); });
		ev.run();
		CPPUNIT_ASSERT_EQUAL(1, calls);
	}

	void testDeferredStopPreservesRemainingWork()
	{
		NET::EventHandler ev;
		int calls = 0;
		ev.defer([&] { ++calls; ev.stop(); });
		ev.defer([&] { ++calls; });

		ev.run();
		CPPUNIT_ASSERT_EQUAL(1, calls);
		ev.resetStop();
		CPPUNIT_ASSERT(ev.wait(0) > 0);
		CPPUNIT_ASSERT_EQUAL(2, calls);
	}

	void testStopDuringRunStartupNotLost()
	{
		for (int i = 0; i < 100; ++i) {
			NET::EventHandler ev;
			std::thread thread([&] { ev.run(); });
			ev.stop();
			thread.join();
		}
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION(EventHandler_TEST);
