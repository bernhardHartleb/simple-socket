#include <simple_socket/TCPSocket.h>

#include <array>
#include <chrono>
#include <thread>
#include <iostream>

using namespace std::chrono_literals;

int main()
{
	while(true)
	{
		try
		{
			// Use a fresh socket for every connection attempt
			NET::TCPSocket socket{NET::IPv4};

			// Attempt to connect within 5s timeout
			const auto connection = socket.timedConnect("127.0.0.1", 47777, 5000);

			if (connection != NET::ConnectResult::Connected)
			{
				std::cerr << "Connect failed\n";
				std::this_thread::sleep_for(3s);
				continue;
			}

			std::cout << "Connected to "
			          << socket.getForeignAddress() << ":"
			          << socket.getForeignPort() << "\n";

			std::array<char, 8192> buffer;

			while(1)
			{
				// Expect to recevie initial data after at least 2s
				const auto result = socket.timedReceive(buffer.data(), buffer.size(), 2000);

				if (result.status == NET::IOStatus::Success)
				{
					std::cout << "Received data from server: ";
					// Received network data is not necessarily null terminated
					std::cout.write(buffer.data(), result.bytes);
					std::cout << std::endl;
					continue;
				}
				else if (result.status == NET::IOStatus::PeerDisconnected)
				{
					std::cerr << "Server disconnected; reconnecting\n";
					break;
				}
				else
				{
					// No data within the deadline, or another error
					std::cerr << "Receive timeout; reconnecting\n";
					break;
				}
			}

			// socket destructor closes the connection here
		}
		catch (const NET::SocketException& e)
		{
			std::cerr << "Socket error: " << e.what() << '\n';
			return 1;
		}

		std::this_thread::sleep_for(1s);
	}
}
