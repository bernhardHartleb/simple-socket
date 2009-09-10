#include "../UDPSocket.h"

#include <cstring>
#include <iostream>

using std::cout;
using std::endl;
using namespace NET;

int main()
{
	static char send_msg[] = "The quick brown fox jumps over the lazy dog";
	static char recv_msg[sizeof(send_msg)];
	static size_t len = sizeof(send_msg);


	// Test peer status on a connected UDP socket
	UDPSocket peer_socket;
	peer_socket.connect( "127.0.0.1", 65001);
	if( peer_socket.peerDisconnected()) return 1;
	peer_socket.send( "", 1); // ok, no icmp received yet
	peer_socket.send( "", 1); // icmp received, send fails
	if( !peer_socket.peerDisconnected()) return 1;


	// Test UDP specific calls
	UDPSocket send_socket(47776);
	UDPSocket recv_socket(47777);
	std::string source;
	unsigned short port = 0;

	send_socket.sendTo( send_msg, len, "127.0.0.1", 47777);
	recv_socket.receiveFrom( recv_msg, len, source, port);

	if( port != 47776) return 1;
	if( source != "127.0.0.1") return 1;
	if( recv_socket.receiveFrom( recv_msg, len, source, port, 10) != 0) return 1;

	// source must not be changed
	if( port != 47776) return 1;
	if( source != "127.0.0.1") return 1;
	if( std::memcmp( send_msg, recv_msg, len) < 0) return 1;
	std::memset( recv_msg, 0, len);


	// Test multicast
	send_socket.setMulticastTTL(0);
	send_socket.connect( "224.40.0.1", 47777);
	recv_socket.joinGroup("224.40.0.1");
	send_socket.send( send_msg, len);
	recv_socket.receive( recv_msg, len);

	if( std::memcmp( send_msg, recv_msg, len) < 0) return 1;
	std::memset( recv_msg, 0, len);

	recv_socket.leaveGroup("224.40.0.1");
	send_socket.send( send_msg, len);

	if( recv_socket.timedReceive( recv_msg, len, 0) != 0) return 1;
}
