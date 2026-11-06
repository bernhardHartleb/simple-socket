#include <simple_socket/TCPSocket.h>

#include <iostream>

const char hello_msg[] = "The quick brown fox jumps over the lazy dog";
const size_t hello_len = sizeof(hello_msg) - 1;

int main()
{
	try {

	NET::TCPSocket server_socket;
	// Allows server to bind again while stale connections are waiting
	server_socket.setReuseAddress(true);

	// Listen locally on the given port
	server_socket.bind("127.0.0.1", 47777);
	server_socket.listen();

	while(true)
	{
		std::cout << "Waiting for a client...\n";

		// Wait up to 5 seconds for an incoming connection
		auto handle = server_socket.timedAccept(5000);
		if (!handle)
		{
			std::cerr << "Timed out waiting for a client.\n";
			return 1;
		}

		// The accepted handle owns the new connection until it is moved
		NET::TCPSocket session = std::move(*handle);

		std::cout << "Client connected from "
		          << session.getForeignAddress() << ":"
		          << session.getForeignPort() << "\n";

		// sendAll() keeps sending until the complete buffer was handled
		NET::IOResult sent = session.sendAll(hello_msg, hello_len);

		if (sent.status == NET::IOStatus::PeerDisconnected)
		{
			std::cerr << "Client disconnected while sending.\n";
			return 1;
		}

		// Gracefully signal that no more data will be sent
		// The socket could still receive data after this call
		session.shutdown(NET::STOP_SEND);

		std::cout << "Successfully sent " << sent.bytes << " bytes.\n";
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
