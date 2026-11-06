#include <simple_socket/TCPSocket.h>
#include <simple_socket/SocketUtils.h>

#include <array>
#include <iostream>

int main()
{
	try {

	const std::string server_ip = NET::resolveHostname("localhost");
	std::cout << "Using server: " << server_ip << "\n";

	NET::TCPSocket client_socket;
	// Connect to local TCP Server example
	client_socket.connect(server_ip, 47777);

	std::array<char, 256> buffer{};

	while(true)
	{
		// Wait for some data but bail out after some time
		NET::IOResult result = client_socket.timedReceive(buffer.data(), buffer.size(), 5000);

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
			// Graceful EOF from the server
			break;
		}

		std::cerr << "\nTimed out waiting for server data.\n";
		return 1;
	}

	// No explicit close is necessary
	return 0;

	}
	catch (const NET::SocketException& e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
}
