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

SimpleSocket::~SimpleSocket()
{
	::close(m_socket);
	// on Windows do cleanup here
}

int SimpleSocket::send( const void* buffer, int len)
{
	int sent;
	if( (sent = ::send( m_socket, (raw_type*) buffer, len, 0)) < 0)
		throw SocketException("Send failed (send)");
	return sent;
}

int SimpleSocket::receive( void* buffer, int len)
{
	int rtn;
	if( (rtn = ::recv( m_socket, (raw_type*) buffer, len, 0)) < 0)
		throw SocketException("Received failed (receive)");
	if( !rtn)
		m_peerDisconnected = true;
	return rtn;
}

int SimpleSocket::timedReceive( void* buffer, int len, int timeout)
{
	int rtn;
	struct pollfd poll;
	poll.fd = m_socket;
	poll.events = POLLIN | POLLPRI | POLLRDHUP;

	rtn = ::poll( &poll, 1, timeout);
	
	if( poll.revents & POLLRDHUP)
	{
		m_peerDisconnected = true;
		return 0;
	}	
	
	if( rtn == 0)
		return 0;
	if( rtn < 0)
		throw SocketException("Receive failed (poll)");

	if( (rtn = ::recv( m_socket, (raw_type*) buffer, len, 0)) < 0)
		throw SocketException("Receive failed (receive)");
	if( !rtn)
		m_peerDisconnected = true;
	return rtn;
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

SimpleSocket::SimpleSocket( int sockfd)
: m_socket(sockfd)
, m_peerDisconnected(false) {}
