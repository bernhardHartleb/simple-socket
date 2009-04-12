#include "UnixDatagramSocket.h"

#include <sys/types.h>		// for data types
#include <sys/socket.h>		// for socket(), connect(), send(), and recv()
#include <arpa/inet.h>		// for inet_addr()
#include <sys/un.h>		// for sockaddr_un
#include <cstring>		// for strcpy()
typedef void raw_type;		// type used for raw data on this platform

using namespace NET;

UnixDatagramSocket::UnixDatagramSocket()
: UnixSocket( DATAGRAM, 0)
{}

UnixDatagramSocket::UnixDatagramSocket( const std::string& localPath)
: UnixSocket( DATAGRAM, 0)
{
	bind(localPath);
}

void UnixDatagramSocket::sendTo( const void* buffer, int len, const std::string& foreignPath)
{
	sockaddr_un destAddr;
	destAddr.sun_family = AF_LOCAL;
	std::strcpy( destAddr.sun_path, foreignPath.c_str());

	// Write out the whole buffer as a single message.
	if( sendto( m_socket, (raw_type*)buffer, len, 0, (sockaddr*)&destAddr, sizeof(destAddr)) != len)
		throw SocketException("Send failed (sendto)");
}

int UnixDatagramSocket::recvFrom( void* buffer, int len, std::string& sourcePath)
{
	sockaddr_un clientAddr;
	socklen_t addrLen = sizeof(clientAddr);
	int rtn = recvfrom( m_socket, (raw_type*)buffer, len, 0, (sockaddr*)&clientAddr, (socklen_t*)&addrLen);
	if( rtn < 0)
		throw SocketException("Receive failed (recvfrom)");

	sourcePath = clientAddr.sun_path;
	return rtn;
}
