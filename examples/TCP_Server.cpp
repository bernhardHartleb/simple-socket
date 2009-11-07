#include <TCPSocket.h>

#include <iostream>

using namespace NET;

static const char send_msg[] = "The quick brown fox jumps over the lazy dog";

int main()
{
	try {

	TCPSocket server_socket;
	server_socket.bind( "127.0.0.1", 47777);
	server_socket.listen();

	TCPSocket::Handle handle = server_socket.timedAccept(5000);
	// nobody connected within 5s
	if(!handle) return 1;

	// create a new socket to communicate with the connected client
	// if we do nothing at this point, the connection will be discarded
	TCPSocket session_socket(handle);
	session_socket.send( send_msg, sizeof(send_msg) );

	// client disconnected before he could receive anything
	if( session_socket.peerDisconnected()) return 1;

	std::cout << "TCP server sucessfully sent data to client." << std::endl;

	// this is optional, we could simply do nothing here
	session_socket.disconnect();

	// if something fails, we get here
	} catch( const SocketException& e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}

	return 0;
}
