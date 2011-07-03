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
	sockaddr_un addr;
	fillAddress( foreignPath, addr);

	if( ::connect( m_socket, (sockaddr*) &addr, sizeof(addr)) < 0)
		throw SocketException("Connect failed (connect)");
}

void UnixSocket::bind( const std::string& localPath)
{
	sockaddr_un addr;
	fillAddress( localPath, addr);
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

	return extractPath( addr, addr_len);
}

std::string UnixSocket::getForeignPath() const
{
	sockaddr_un addr;
	socklen_t addr_len = sizeof(addr);

	if( getpeername( m_socket, (sockaddr*) &addr, &addr_len) < 0)
		throw SocketException("Fetch of foreign path failed (getpeername)");

	return extractPath( addr, addr_len);
}

void UnixSocket::fillAddress( const std::string& path, sockaddr_un& addr)
{
	// needed space is size plus null character
	if( path.size() >= sizeof(sockaddr_un::sun_path))
		throw SocketException("Path to socket file is too long", false);

	addr.sun_family = AF_LOCAL;
	std::strcpy( addr.sun_path, path.c_str());
}

std::string UnixSocket::extractPath( const sockaddr_un& addr, socklen_t len)
{
	return std::string( addr.sun_path, len - sizeof(sa_family_t) - 1);
}
