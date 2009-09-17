#include "UnixSocket.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <cstring>

using namespace NET;

UnixSocket::UnixSocket( int type, int protocol)
: SimpleSocket( UNIX, type, protocol)
{}

void UnixSocket::connect( const std::string& foreignPath)
{
	if( !isValidPath(foreignPath) )
		throw SocketException("foreignPath is too big", false);

	sockaddr_un addr;
	addr.sun_family = AF_LOCAL;
	std::strcpy( addr.sun_path, foreignPath.c_str());

	if( ::connect( m_socket, (sockaddr*) &addr, sizeof(addr)) < 0)
		throw SocketException("Connect failed (connect)");
}

void UnixSocket::disconnect()
{
	sockaddr_un addr;
	std::memset( &addr, 0, sizeof(addr));
	addr.sun_family = AF_UNSPEC;

	if( ::connect( m_socket, (sockaddr*) &addr, sizeof(addr)) < 0)
		throw SocketException("Disconnect failed (connect)");
}

void UnixSocket::bind( const std::string& localPath)
{
	if( !isValidPath(localPath) )
		throw SocketException("localPath is too big", false);

	sockaddr_un addr;
	addr.sun_family = AF_LOCAL;
	std::strcpy( addr.sun_path, localPath.c_str());
	::unlink( addr.sun_path);

	if( ::bind( m_socket, (sockaddr*) &addr, sizeof(addr)) < 0)
		throw SocketException("Set of local path failed (bind)");
}

std::string UnixSocket::getLocalPath() const
{
	sockaddr_un addr;
	socklen_t addr_len = sizeof(addr);

	if( getsockname( m_socket, (sockaddr*) &addr, &addr_len) < 0)
		throw SocketException("Fetch of local path failed (getsockname)");

	return addr.sun_path;
}

std::string UnixSocket::getForeignPath() const
{
	sockaddr_un addr;
	socklen_t addr_len = sizeof(addr);

	if( getpeername( m_socket, (sockaddr*) &addr, &addr_len) < 0)
		throw SocketException("Fetch of foreign path failed (getpeername)");

	return addr.sun_path;
}

bool UnixSocket::isValidPath( const std::string& path)
{
	if( path.size() >= 104) return false;
	else return true;
}
