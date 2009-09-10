#include "UnixSocket.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <cstring>
#include <cerrno>

using namespace NET;

UnixSocket::UnixSocket( int type, int protocol)
: SimpleSocket( UNIX, type, protocol)
{}

void UnixSocket::connect( const std::string& path)
{
	sockaddr_un addr;
	addr.sun_family = AF_LOCAL;
	std::strcpy( addr.sun_path, path.c_str());

	// Try to connect to the given port
	if( ::connect( m_socket, (sockaddr*) &addr, sizeof(addr)) < 0)
		throw SocketException("Connect failed (connect)");
}

void UnixSocket::disconnect()
{
	sockaddr_un addr;
	std::memset( &addr, 0, sizeof(addr));
	addr.sun_family = AF_UNSPEC;

	// Try to disconnect
	if( ::connect( m_socket, (sockaddr*) &addr, sizeof(addr)) < 0) {
		if( errno != EAFNOSUPPORT)
			throw SocketException("Disconnect failed (connect)");
	}
}

void UnixSocket::bind( const std::string& localPath)
{
	sockaddr_un addr;
	addr.sun_family = AF_LOCAL;
	std::strcpy( addr.sun_path, localPath.c_str());
	::unlink( addr.sun_path);

	if( ::bind( m_socket, (sockaddr*) &addr, sizeof(addr)) < 0)
		throw SocketException("Set of local path failed (bind)");
}

std::string UnixSocket::getLocalPath()
{
	sockaddr_un addr;
	socklen_t addr_len = sizeof(addr);

	if( getsockname( m_socket, (sockaddr*) &addr, &addr_len) < 0)
		throw SocketException("Fetch of local path failed (getsockname)");

	return addr.sun_path;
}

std::string UnixSocket::getForeignPath()
{
	sockaddr_un addr;
	socklen_t addr_len = sizeof(addr);

	if( getpeername( m_socket, (sockaddr*) &addr, &addr_len) < 0)
		throw SocketException("Fetch of foreign path failed (getpeername)");
	return addr.sun_path;
}
