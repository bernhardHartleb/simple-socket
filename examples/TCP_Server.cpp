#include "../TCPSocket.h"

#include <iostream>

static const char send_msg[] = "The quick brown fox jumps over the lazy dog";
static const size_t send_len = sizeof(send_msg) - 1;

int main()
{
	try {

	NET::TCPSocket server_socket;
	// Allow server to bind again while stale connections are waiting
	server_socket.setReuseAddress(true);
	// Listen locally on the given port
	server_socket.bind("127.0.0.1", 47777);
	server_socket.listen();

	std::cout << "Waiting for a client..." << std::endl;

	// Wait up to 5 seconds for an incoming connection
	NET::TCPSocket::Handle handle = server_socket.timedAccept(5000);
	if (!handle)
	{
		std::cerr << "Timed out waiting for a client." << std::endl;
		return 1;
	}

	// The accepted handle owns the new connection until it is moved into a TCPSocket
	NET::TCPSocket session_socket(std::move(handle));

	std::cout << "Client connected from " << session_socket.getForeignAddress()
	          << ":" << session_socket.getForeignPort() << std::endl;

	// TCP is a byte stream. send() may send only part of a buffer
	// sendAll() keeps sending until the complete buffer was handled
	int sent = session_socket.sendAll(send_msg, send_len);
	if (sent != static_cast<int>(send_len))
	{
		std::cerr << "Client disconnected while sending." << std::endl;
		return 1;
	}

	// Gracefully signal that no more data will be sent
	// The socket could still receive data after this call
	session_socket.shutdown(NET::SimpleSocket::STOP_SEND);

	std::cout << "Successfully sent " << sent << " bytes." << std::endl;
	// No explicit close is necessary

	}
	catch (const NET::SocketException& e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}

	return 0;
}
