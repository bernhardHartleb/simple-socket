#include "SimpleSocket.h"

#include <sys/types.h>		// for data types
#include <sys/socket.h>		// for socket(), connect(), send(), and recv()
#include <poll.h>
#include <unistd.h>		// for close()
#include <cstring>		// for strerror()
#include <cerrno>		// for errno

using namespace NET;

SocketException::SocketException( const std::string& message, bool inclSysMsg /* = true */) throw()
: m_message(message)
{
	if(inclSysMsg)
	{
		m_message += ": ";
		m_message += strerror(errno);
	}
}

SimpleSocket::SimpleSocket( int domain, int type, int protocol)
: m_peerDisconnected(false)
{
	if( (m_socket = socket( domain, type, protocol)) < 0)
		throw SocketException("Socket creation failed (socket)");
}

SimpleSocket::SimpleSocket( int sockfd)
: m_socket(sockfd)
, m_peerDisconnected(false)
{}

SimpleSocket::~SimpleSocket()
{
	::close(m_socket);
	// on Windows do cleanup here
}

int SimpleSocket::send( const void* buffer, size_t len)
{
	int sent;
	if( (sent = ::send( m_socket, (raw_type*) buffer, len, 0)) < 0)
		throw SocketException("Send failed (send)");
	return sent;
}

int SimpleSocket::receive( void* buffer, size_t len)
{
	int ret = ::recv( m_socket, (raw_type*) buffer, len, 0);

	if( ret < 0) throw SocketException("Received failed (receive)");
	if( !ret) m_peerDisconnected = true;
	return ret;
}

int SimpleSocket::timedReceive( void* buffer, size_t len, int timeout)
{
	struct pollfd poll;
	poll.fd = m_socket;
	poll.events = POLLIN | POLLPRI | POLLRDHUP;

	int ret = ::poll( &poll, 1, timeout);

	if( ret == 0) return 0;
	if( ret < 0)  throw SocketException("Receive failed (poll)");

	if( poll.revents & POLLIN || poll.revents & POLLPRI) {
		ret = ::recv( m_socket, (raw_type*) buffer, len, 0);
		if( ret < 0) throw SocketException("Receive failed (receive)");
		if( !ret) m_peerDisconnected = true;
		return ret;
	}

	if( poll.revents & POLLRDHUP)
	{
		m_peerDisconnected = true;
		return 0;
	}

	return 0;
}

void SimpleSocket::shutdown( ShutdownDirection type)
{
	if( ::shutdown( m_socket, type) < 0)
		throw SocketException("Shutdown failed (shutdown)");
}

bool SimpleSocket::peerDisconnected()
{
	return m_peerDisconnected;
}
