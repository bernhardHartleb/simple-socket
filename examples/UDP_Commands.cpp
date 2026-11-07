#include <simple_socket/UDPSocket.h>

#include <array>
#include <iostream>

const char LOOPBACK[] = "127.0.0.1";
const unsigned short PORT = 47778;

static int runServer()
{
	NET::UDPSocket socket;
	socket.bind(LOOPBACK, PORT);

	std::cout << "Listening for UDP commands on " << LOOPBACK << ":" << PORT << std::endl;
	std::array<char, 256> buffer{};

	while(true)
	{
		std::string sourceAddress;
		unsigned short sourcePort = 0;

		NET::IOResult result = socket.receiveFrom(buffer.data(), buffer.size(), sourceAddress, sourcePort);
		if (result.bytes == 0) continue;

		std::string command(buffer.data(), result.bytes);
		std::cout << "Received from " << sourceAddress << ":" << sourcePort << " " << command << std::endl;

		if (command == "quit")
		{
			std::cout << "Stopping." << std::endl;
			break;
		}
	}
	return 0;
}

static int sendCommand(const std::string& command)
{
	NET::UDPSocket socket;
	NET::IOResult result = socket.sendTo(command.data(), command.size(), LOOPBACK, PORT);
	if (result) std::cout << "Sent command: " << command << std::endl;
	return 0;
}

int main(int argc, char* argv[])
{
	try
	{
		if (argc == 2 && std::string(argv[1]) == "server")
		{
			return runServer();
		}
		if (argc == 3 && std::string(argv[1]) == "send")
		{
			return sendCommand(argv[2]);
		}

		std::cerr
			<< "Usage:\n"
			<< "  " << argv[0] << " server\n"
			<< "  " << argv[0] << " send <command>\n";

		return 1;
	}
	catch (const NET::SocketException& e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
}
