#include "../UDPSocket.h"

#include <iostream>

static const char send_msg[] = "Welcome to our multicast group!";
static char recv_msg[sizeof(send_msg)];
static size_t len = sizeof(send_msg);

int main()
{
	try {

	NET::UDPSocket recv_socket;
	// listen on the given port and any address
	recv_socket.bind(47777);
	// receive data from our multicast group
	recv_socket.joinGroup("224.40.0.1");

	NET::UDPSocket send_socket;
	// don't route our packets
	send_socket.setMulticastTTL(0);
	// optional, the OS would select a sender port
	send_socket.bind(47776);


	int bytes;
	std::string source;
	unsigned short port = 0;

	// send data to a specific destination
	send_socket.sendTo( send_msg, len, "224.40.0.1", 47777);

	// receive data from with sender information
	bytes = recv_socket.receiveFrom( recv_msg, len, source, port);

	if(bytes > 0)
		std::cout << source << ":" << port << " " << recv_msg << std::endl;


	// set default address for sending
	send_socket.connect( "224.40.0.1", 47777);

	send_socket.send( send_msg, len);
	bytes = recv_socket.receive( recv_msg, len);

	if(bytes > 0)
		std::cout << "We got another message." << std::endl;

	// this is optional and just for demonstration
	recv_socket.leaveGroup("224.40.0.1");


	// if something fails, we get here
	} catch( const NET::SocketException& e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}

	return 0;
}
