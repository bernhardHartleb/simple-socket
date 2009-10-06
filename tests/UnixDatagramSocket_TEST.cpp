#include "../UnixDatagramSocket.h"

#include <cstring>
#include <iostream>

using namespace NET;

int main()
{
	static char sock_file[] = "/tmp/simple-socket_test.sock";
	static char send_msg[] = "The quick brown fox jumps over the lazy dog";
	static char recv_msg[sizeof(send_msg)];
	static size_t len = sizeof(send_msg);

	try {

	// Test datagram specific calls
	UnixDatagramSocket send_socket;
	UnixDatagramSocket recv_socket;
	send_socket.bind(sock_file);
	recv_socket.bind(sock_file);
	std::string source;

	send_socket.sendTo( send_msg, len, sock_file);
	recv_socket.receiveFrom( recv_msg, len, source);

	if( source != sock_file) return 1;
	if( recv_socket.timedReceiveFrom( recv_msg, len, source, 10) != 0) return 1;

	// source must not be changed
	if( source != sock_file) return 1;
	if( std::memcmp( send_msg, recv_msg, len) < 0) return 1;
	std::memset( recv_msg, 0, len);

	::unlink(sock_file);

	// no errors in this testcase
	} catch( const SocketException& e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << e.errorCode() << std::endl;
		return 1;
	}
}
