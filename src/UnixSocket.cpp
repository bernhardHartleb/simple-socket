#include "UnixSocket.h"
#include "SockAddress.h"

#include <sys/socket.h>
#include <unistd.h>

using namespace NET;

UnixSocket::UnixSocket( int type, int protocol)
: SimpleSocket( AF_UNIX, type, protocol)
{}

UnixSocket::UnixSocket( int sockfd)
: SimpleSocket(sockfd)
{}

UnixSocket::~UnixSocket()
{
	if( !m_boundPath.empty())
		::unlink( m_boundPath.c_str());
}

void UnixSocket::connect( std::string_view foreignPath)
{
	SockAddress addr( foreignPath);

	if( ::connect( m_socket, addr.data(), addr.length()) < 0)
		throw SocketException("Connect failed (connect)");

	m_peerDisconnected = false;
}

void UnixSocket::bind( std::string_view localPath)
{
	SockAddress addr( localPath);

	if( ::bind( m_socket, addr.data(), addr.length()) < 0)
		throw SocketException("Set of local path failed (bind)");

	m_boundPath = localPath;
}

std::string UnixSocket::getLocalPath() const
{
	SockAddress addr;

	if( ::getsockname( m_socket, addr.data(), addr.lengthPtr()) < 0)
		throw SocketException("Fetch of local path failed (getsockname)");

	return addr.address();
}

std::string UnixSocket::getForeignPath() const
{
	SockAddress addr;

	if( ::getpeername( m_socket, addr.data(), addr.lengthPtr()) < 0)
		throw SocketException("Fetch of foreign path failed (getpeername)");

	return addr.address();
}
