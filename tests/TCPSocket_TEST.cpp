#include "../TCPSocket.h"

#include <cstring>
#include <iostream>

using namespace NET;

int main()
{
	static char send_msg[] = "The quick brown fox jumps over the lazy dog";
	static char recv_msg[sizeof(send_msg)];
	static size_t len = sizeof(send_msg);

	try {

	// Test peer status on a TCP socket
	TCPSocket server_socket;
	TCPSocket client_socket;
	server_socket.bind( "127.0.0.1", 47777);
	server_socket.listen();
	if( server_socket.peerDisconnected()) return 1;

	client_socket.connect( "127.0.0.1", 47777);

	const TCPSocket::Handle& handle = server_socket.accept();
	if(!handle) return 1;
	TCPSocket session_socket(handle);
	if( session_socket.peerDisconnected()) return 1;

	session_socket.send( send_msg, len);
	if( client_socket.receive( recv_msg, len) != len) return 1;
	if( std::memcmp( send_msg, recv_msg, len) < 0) return 1;

	client_socket.disconnect();
	session_socket.send( send_msg, len);
	if( !session_socket.peerDisconnected()) return 1;

	// no errors in this testcase
	} catch( const SocketException& e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << e.errorCode() << std::endl;
		return 1;
	}
}
