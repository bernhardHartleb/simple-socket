#include <simple_socket/EventHandler.h>

#include <chrono>
#include <iostream>
#include <thread>

using namespace std::chrono_literals;

int main()
{
	NET::EventHandler events;

	// One-shot task.
	events.addOneShotTimer(600ms, [](std::uint64_t) {
		std::cout << "one-shot timer fired\n";
	});

	// Periodic heartbeat. The callback receives the number of expirations
	// accumulated since the previous read, even when the application is busy.
	const auto heartbeat = events.addPeriodicTimer(500ms, [](std::uint64_t expirations) {
		std::cout << "heartbeat (" << expirations << " expiration)\n";
	});

	int tick = 0;
	NET::EventHandler::TimerId timer = 0;
	// One-shot timer restarting itself and performing deferred idle work.
	timer = events.addOneShotTimer(100ms, [&](std::uint64_t) {
		const int current = ++tick;
		std::cout << "timer fired: " << current << '\n';

		// Runs after the current ready-I/O/timer batch.
		events.defer([&, current] {
			std::cout << "  deferred work for tick " << current << '\n';
		});

		// Reuse the same timerfd and TimerId, even from its own callback.
		if(current < 3) events.restartTimer(timer, 100ms);
	});

	// Demonstrate stop() from another thread.
	std::thread shutdown([&events] {
		std::this_thread::sleep_for(1100ms);
		events.stop(); // Wakes event loop via the internal eventfd.
	});

	try {
		events.run();
	} catch (...) {
		events.stop();
		shutdown.join();
		throw;
	}

	shutdown.join();
	events.removeTimer(heartbeat);
	std::cout << "event loop exited\n";
	return 0;
}
