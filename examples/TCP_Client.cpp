#include <TCPSocket.h>

#include <iostream>

using namespace NET;

static char recv_msg[50];

int main()
{
	try {

	TCPSocket client_socket;
	client_socket.connect( "127.0.0.1", 47777);

	// wait for data within the next 5s
	int bytes_recv = client_socket.timedReceive( recv_msg, 50, 5000);

	// we got a message!
	if(bytes_recv > 0)
		std::cout << "Received data from server: " << recv_msg << std::endl;

	// this is optional, we could simply do nothing here
	client_socket.disconnect();

	// if something fails, we get here
	} catch( const SocketException& e)
	{
		std::cerr << e.what() << e.errorCode() << std::endl;
		return 1;
	}

	return 0;
}
