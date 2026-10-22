#include "UnixSocket.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>

using namespace NET;

UnixSocket::UnixSocket( int type, int protocol)
: SimpleSocket( UNIX, type, protocol)
{}

UnixSocket::~UnixSocket()
{
	if( !m_boundPath.empty())
		::unlink( m_boundPath.c_str());
}

void UnixSocket::connect( std::string_view foreignPath)
{
	sockaddr_un addr;
	fillAddress( foreignPath, addr);

	if( ::connect( m_socket, (sockaddr*) &addr, sizeof(addr)) < 0)
		throw SocketException("Connect failed (connect)");
}

void UnixSocket::bind( std::string_view localPath)
{
	sockaddr_un addr;
	fillAddress( localPath, addr);

	if( ::bind( m_socket, (sockaddr*) &addr, sizeof(addr)) < 0)
		throw SocketException("Set of local path failed (bind)");

	m_boundPath = localPath;
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

void UnixSocket::fillAddress( std::string_view path, sockaddr_un& addr)
{
	const size_t len = path.length();
	// needed space is size plus null character
	if( len >= sizeof(addr.sun_path))
		throw SocketException("Path to socket file is too long", false);

	addr = {};
	addr.sun_family = AF_LOCAL;
	std::memcpy( addr.sun_path, path.data(), len);
	addr.sun_path[len] = 0;
}

std::string UnixSocket::extractPath( const sockaddr_un& addr, socklen_t len)
{
	const size_t offset = offsetof(sockaddr_un, sun_path);
	if( len <= offset)
		return std::string();

	size_t path_len = static_cast<size_t>(len) - offset;
	if( path_len > sizeof(addr.sun_path))
		path_len = sizeof(addr.sun_path);

	if( path_len > 0 && addr.sun_path[path_len - 1] == '\0')
		--path_len;

	return std::string( addr.sun_path, path_len);
}
