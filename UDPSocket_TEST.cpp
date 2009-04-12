#include "UDPSocket.h"

#include <cstring>
#include <iostream>

using std::cout;
using std::endl;
using namespace NET;

int main()
{
	cout << "Testing UDPSocket ..." << endl;

	static char send_msg[] = "The quick brown fox jumps over the lazy dog";
	static char recv_msg[sizeof(send_msg)];

	UDPSocket send_socket;
	send_socket.setMulticastTTL(0);
	send_socket.connect( "224.40.0.1", 4777);

	UDPSocket recv_socket(4777);
	recv_socket.joinGroup("224.40.0.1");

	send_socket.send( send_msg, sizeof(send_msg));
	cout << "send: " << send_msg << endl;

	recv_socket.receive( recv_msg, sizeof(send_msg));
	cout << "recv: " << recv_msg << endl;

	if( std::memcmp( send_msg, recv_msg, sizeof(send_msg))) return 1;
}
