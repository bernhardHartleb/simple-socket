#include "../UDPSocket.h"

#include <array>
#include <iostream>

static const char multicast_group[] = "239.255.0.1";
static const unsigned short multicast_port = 47777;
static const char send_msg[] = "Welcome to our multicast group!";
static const int send_len = sizeof(send_msg) - 1;

int main()
{
	try {

	NET::UDPSocket recv_socket;
	// Allow multiple multicast receivers on this port (all get a copy)
	recv_socket.setReuseAddress(true);
	// Listen on the multicast port on any IPv4 address
	recv_socket.bind(multicast_port);
	// Join the multicast group so packets addressed to the group can be delivered to this socket
	recv_socket.joinGroup(multicast_group);

	NET::UDPSocket send_socket;
	// TTL 0 keeps packets self-contained on the local host
	send_socket.setMulticastTTL(0);
	// Send to group
	send_socket.sendTo(send_msg, send_len, multicast_group, multicast_port);

	std::array<char, 256> buffer{};
	std::string source;
	unsigned short source_port = 0;

	// UDP preserves datagram boundaries
	// one receiveFrom() corresponds to one datagram
	int received = recv_socket.timedReceiveFrom(buffer.data(), buffer.size(), source, source_port, 1000);
	if (received == 0)
	{
		std::cerr << "Timed out waiting for multicast datagram." << std::endl;
		return 1;
	}

	std::cout << "Received from " << source << ":" << source_port << ": ";
	std::cout.write(buffer.data(), received);
	std::cout << std::endl;

	// Not required before destruction, but shown because leaving
	// multicast groups explicitly can be useful
	recv_socket.leaveGroup(multicast_group);

	}
	catch (const NET::SocketException& e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}

	return 0;
}
