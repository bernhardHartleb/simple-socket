#include <simple_socket/EventHandler.h>
#include <simple_socket/UDPSocket.h>

#include <array>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

// Three independent UDP receivers, dispatched by one event loop.
// Send "quit" to any port to stop the example.

int main()
{
	try {

	const std::array<unsigned short, 3> Ports {47777, 47778, 47779};
	const int MaxDatagramsPerEvent = 32;

	std::array<NET::UDPSocket, Ports.size()> sockets;
	NET::EventHandler events;

	for (std::size_t i = 0; i < sockets.size(); ++i)
	{
		auto& socket = sockets[i];
		socket.bind("127.0.0.1", Ports[i]);
		socket.setBlocking(false);

		events.add(socket.nativeHandle(), NET::EventHandler::Read,
		           [&, i](int, NET::EventHandler::Events flags)
		{
			// Error and Hup are delivered regardless of subscribed interests
			if (flags & (NET::EventHandler::Error | NET::EventHandler::Hangup))
				throw std::runtime_error("UDP socket error on port " + std::to_string(Ports[i]));

			// Level-triggered: read queued datagrams until WouldBlock
			// Limit work per callback so busy ports don't starve others

			std::array<char, 4096> buffer {};
			for (int n = 0; n < MaxDatagramsPerEvent; ++n)
			{
				std::string sender;
				unsigned short senderPort = 0;
				const auto result = sockets[i].receiveFrom(buffer.data(), buffer.size(), sender, senderPort);

				if (result.status == NET::IOStatus::WouldBlock) break;
				if (result.status != NET::IOStatus::Success)
					throw std::runtime_error("UDP receive failed on port " + std::to_string(Ports[i]));

				std::cout << "UDP " << Ports[i] << " <- "
							<< sender << ':' << senderPort
							<< " (" << result.bytes << " bytes): \"";
				std::cout.write(buffer.data(), result.bytes);
				std::cout << "\"" << std::endl;

				if (std::string_view(buffer.data(), result.bytes) == "quit") {
					events.stop();
					return;
				}
			}
		});

		std::cout << "Listening on 127.0.0.1:" << Ports[i] << '\n';
	}

	std::cout << "One event loop. Send 'quit' to any port to stop." << std::endl;
	events.run();
	return 0;

	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
}
