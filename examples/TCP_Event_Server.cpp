#include <simple_socket/EventHandler.h>
#include <simple_socket/TCPSocket.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

// Event-driven TCP echo server for multiple clients using simple-socket in a single thead.

class EchoServer
{
	static constexpr std::size_t MaxPending = 256 * 1024;

	struct Client
	{
		explicit Client(NET::TCPSocket&& s) : socket(std::move(s)) {}
		NET::TCPSocket socket;
		std::string output;
		std::size_t sent = 0;
		bool inputClosed = false;
		std::size_t queued() const noexcept { return output.size() - sent; }
	};

public:
	explicit EchoServer(unsigned short port)
	{
		m_listener.setReuseAddress(true);
		m_listener.bind("127.0.0.1", port);
		m_listener.listen();
		m_listener.setBlocking(false); // Inherited by clients

		m_events.add(m_listener.nativeHandle(), NET::EventHandler::Read,
			[this](int, NET::EventHandler::Events flags) { onAccept(flags); });

		std::cout << "Listening on 127.0.0.1:" << m_listener.getLocalPort() << "\n";
	}

	void run() { m_events.run(); }
	void stop() { m_events.stop(); }

private:
	void onAccept(NET::EventHandler::Events flags)
	{
		if (flags & (NET::EventHandler::Error | NET::EventHandler::Hangup))
			throw std::runtime_error("Listening socket failure");

		// Accept all queued connections without blocking.
		while (auto incoming = m_listener.accept())
		{
			const int fd = incoming->nativeHandle();
			auto client = std::make_unique<Client>(std::move(*incoming));

			// Attempt to add first before adding client to our list.
			m_events.add(fd, NET::EventHandler::Read | NET::EventHandler::PeerClosed,
				[this](int clientFd, NET::EventHandler::Events flags) { onClient(clientFd, flags); });

			m_clients.emplace(fd, std::move(client));
			std::cout << "Connected fd=" << fd << "\n";
		}
	}

	void closeClient(int fd)
	{
		m_events.remove(fd); // Unregister before destroying the descriptor.
		m_clients.erase(fd);
		std::cout << "Closed fd=" << fd << "\n";
	}

	void readClient(Client& client)
	{
		char buffer[8192];
		while (!client.inputClosed && client.queued() < MaxPending)
		{
			const auto capacity = std::min(sizeof(buffer), MaxPending - client.queued());
			const NET::IOResult result = client.socket.receive(buffer, capacity);

			if (result.status == NET::IOStatus::Success) {
				// Compact already-sent bytes before appending more data.
				if (client.sent != 0) {
					client.output.erase(0, client.sent);
					client.sent = 0;
				}
				client.output.append(buffer, result.bytes);
			} else if (result.status == NET::IOStatus::WouldBlock) {
				break;
			} else if (result.status == NET::IOStatus::PeerDisconnected) {
				client.inputClosed = true;
				break;
			} else {
				throw std::runtime_error("Unexpected TCP receive status");
			}
		}
	}

	void writeClient(Client& client)
	{
		while (client.queued() != 0)
		{
			const auto position = client.output.data() + client.sent;
			const NET::IOResult result = client.socket.send(position, client.queued());

			if (result.status == NET::IOStatus::Success) {
				if (result.bytes == 0) break; // Guard against no-progress loops.
				client.sent += result.bytes;
			} else if (result.status == NET::IOStatus::WouldBlock) {
				break;
			} else if (result.status == NET::IOStatus::PeerDisconnected) {
				throw std::runtime_error("Peer disconnected during send");
			} else {
				throw std::runtime_error("Unexpected TCP send status");
			}
		}
		if (client.queued() == 0) {
			client.output.clear();
			client.sent = 0;
		}
	}

	void onClient(int fd, NET::EventHandler::Events flags)
	{
		const auto it = m_clients.find(fd);
		if (it == m_clients.end()) return;
		Client& client = *it->second;

		try {
			// Read/drain before reacting to peer shutdown; FIN may accompany data.
			if (!client.inputClosed &&
				(flags & (NET::EventHandler::Read | NET::EventHandler::PeerClosed |
				          NET::EventHandler::Hangup | NET::EventHandler::Error)))
				readClient(client);

			if (client.queued() != 0)
				writeClient(client); // Try immediately, then subscribe to EPOLLOUT.

			if (flags & NET::EventHandler::Error) {
				closeClient(fd);
				return;
			}
			// A hangup without a readable EOF should not lead to repeated wakeups.
			if ((flags & NET::EventHandler::Hangup) && !client.inputClosed) {
				closeClient(fd);
				return;
			}
			if (client.inputClosed && client.queued() == 0) {
				closeClient(fd);
				return;
			}

			NET::EventHandler::Events interests = 0;
			// Pause reads on slow clients; resume as soon as the output drains.
			if (!client.inputClosed && client.queued() < MaxPending)
				interests |= NET::EventHandler::Read | NET::EventHandler::PeerClosed;
			// Pause writes when no more data is queued
			if (client.queued() != 0)
				interests |= NET::EventHandler::Write;

			m_events.modify(fd, interests);

		} catch (const std::exception& e) {
			std::cerr << "Client " << fd << ": " << e.what() << "\n";
			closeClient(fd);
		}
	}

	NET::TCPSocket m_listener;
	NET::EventHandler m_events;
	std::unordered_map<int, std::unique_ptr<Client>> m_clients;
};

int main(int argc, char** argv)
{
	try
	{
		const int port = (argc == 2) ? std::stoi(argv[1]) : 47777;
		if (argc > 2 || port <= 0 || port > 65535)
			throw std::invalid_argument("Usage: TCP_Event_Server [1..65535]");

		EchoServer server(static_cast<unsigned short>(port));
		server.run();
		return 0;
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
}
