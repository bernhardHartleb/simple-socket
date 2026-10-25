#include "../TCPSocket.h"

#include <array>
#include <iostream>

int main()
{
	try {

	NET::TCPSocket client_socket;
	// Connect to local TCP Server example
	client_socket.connect("127.0.0.1", 47777);

	std::array<char, 256> buffer{};
	std::cout << "Received data from server: ";

	while(true)
	{
		// TCP is a byte stream: one receive call is not guaranteed
		// to return everything the peer sent
		int received = client_socket.timedReceive(buffer.data(), buffer.size(), 5000);
		if (received > 0)
		{
			// Use the returned byte count
			// Received network data is not necessarily null terminated
			std::cout.write(buffer.data(), received);
			continue;
		}

		if (client_socket.peerDisconnected())
		{
			// Graceful EOF from the server
			break;
		}

		// timedReceive() also returns 0 when its timeout expires
		std::cerr << "\nTimed out waiting for server data." << std::endl;

		return 1;
	}

	std::cout << std::endl;
	// No explicit close is necessary

	}
	catch (const NET::SocketException& e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}

	return 0;
}
