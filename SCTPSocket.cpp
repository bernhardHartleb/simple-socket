#include "SCTPSocket.h"
#include "SocketUtils.h"

#include <netinet/in.h>
#include <cstring>
#include <poll.h>

using namespace NET;

SCTPSocket::SCTPSocket( unsigned numOutStreams /* = 10 */,
                        unsigned maxInStreams /* = 65535 */,
                        unsigned maxAttempts /* = 4 */,
                        unsigned maxInitTimeout /* = 0 */)
: InternetSocket( STREAM, IPPROTO_SCTP)
{
	setInitValues( numOutStreams, maxInStreams, maxAttempts, maxInitTimeout);
}

SCTPSocket::SCTPSocket( const std::string& foreignAddress,
                        unsigned short foreignPort,
                        unsigned numOutStreams /* = 10 */,
                        unsigned maxInStreams /* = 65535 */,
                        unsigned maxAttempts /* = 4 */,
                        unsigned maxInitTimeout /* = 0 */)
: InternetSocket( STREAM, IPPROTO_SCTP)
{
	setInitValues( numOutStreams, maxInStreams, maxAttempts, maxInitTimeout);
	connect( foreignAddress, foreignPort);
}

SCTPSocket::SCTPSocket( const std::vector<std::string>& foreignAddresses,
                        unsigned short foreignPort,
                        unsigned numOutStreams /* = 10 */,
                        unsigned maxInStreams /* = 65535 */,
                        unsigned maxAttempts /* = 4 */,
                        unsigned maxInitTimeout /* = 0 */)
: InternetSocket( STREAM, IPPROTO_SCTP)
{
	setInitValues( numOutStreams, maxInStreams, maxAttempts, maxInitTimeout);
	sockaddr_in *dest = new sockaddr_in[foreignAddresses.size()];
	int i = 0;
	for( std::vector<std::string>::const_iterator it = foreignAddresses.begin(); it != foreignAddresses.end(); ++it)
	{
		fillAddr( (*it), foreignPort, dest[i++]);
	}
	int ret = sctp_connectx( m_socket, reinterpret_cast<sockaddr*>(dest), foreignAddresses.size());
	delete[] dest;
	if(ret < 0)
		throw SocketException("SCTPSocket::SCTPSocket connect failed");
}

SCTPSocket::SCTPSocket( Handle handle)
: InternetSocket( handle.m_sockfd)
{
	if( handle.m_sockfd == 0)
		throw SocketException("Tried to initialize SCTPSocket with invalid Handle");
}

int SCTPSocket::bind( const std::vector<std::string>& localAddresses, unsigned port /* = 0 */)
{
	sockaddr_in *dest = new sockaddr_in[localAddresses.size()];
	int i = 0;
	for( std::vector<std::string>::const_iterator it = localAddresses.begin(); it != localAddresses.end(); ++it) {
		fillAddr( (*it), port, dest[i++]);
	}
	int ret = sctp_bindx( m_socket, reinterpret_cast<sockaddr*>(dest), localAddresses.size(), SCTP_BINDX_ADD_ADDR);
	delete[] dest;
	if(ret < 0)
		throw SocketException("SCTPSocket::bind bindx failed");
	return ret;
}

int SCTPSocket::state()
{
	struct sctp_status status;
	socklen_t size = sizeof(status);
	if(getsockopt( m_socket, IPPROTO_SCTP, SCTP_STATUS, &status, &size) < 0)
		throw SocketException("SCTPSocket::state failed");
	return status.sstat_state;
}

int SCTPSocket::notAckedData()
{
	struct sctp_status status;
	socklen_t size = sizeof(status);
	if(getsockopt( m_socket, IPPROTO_SCTP, SCTP_STATUS, &status, &size) < 0)
		throw SocketException("SCTPSocket::notAckedData failed");
	return status.sstat_unackdata;
}

int SCTPSocket::pendingData()
{
	struct sctp_status status;
	socklen_t size = sizeof(status);
	if(getsockopt( m_socket, IPPROTO_SCTP, SCTP_STATUS, &status, &size) < 0)
		throw SocketException("SCTPSocket::pendingData failed");
	return status.sstat_penddata;
}

unsigned SCTPSocket::inStreams()
{
	struct sctp_status status;
	socklen_t size = sizeof(status);
	if(getsockopt( m_socket, IPPROTO_SCTP, SCTP_STATUS, &status, &size) < 0)
		throw SocketException("SCTPSocket::inStreams failed");
	return status.sstat_instrms;
}

unsigned SCTPSocket::outStreams()
{
	struct sctp_status status;
	socklen_t size = sizeof(status);
	if(getsockopt( m_socket, IPPROTO_SCTP, SCTP_STATUS, &status, &size) < 0)
		throw SocketException("SCTPSocket::outStreams failed");
	return status.sstat_outstrms;
}

unsigned SCTPSocket::fragmentationPoint()
{
	struct sctp_status status;
	socklen_t size = sizeof(status);
	if(getsockopt( m_socket, IPPROTO_SCTP, SCTP_STATUS, &status, &size) < 0)
		throw SocketException("SCTPSocket::fragmentationPoint failed");
	return status.sstat_fragmentation_point;
}

std::string SCTPSocket::primaryAddress()
{

}

int SCTPSocket::send( const void* data, int length, unsigned stream, unsigned ttl /* = 0 */, unsigned context /* = 0 */,
                      unsigned ppid /* = 0 */, abortFlag abort /* = KEEPALIVE */, switchAddressFlag switchAddr /* = KEEP_PRIMARY */)
{
	int ret = sctp_sendmsg( m_socket, data, length, NULL, 0, ppid, abort + switchAddr, stream, ttl, context);
	if( ret < 0)
		throw SocketException("SCTPSocket::send failed");
	else if (ret < length)
		throw SocketException("Interpreted SCTP false, sent a packet fragement");
	return ret;
}

int SCTPSocket::sendUnordered(  const void* data, int length, unsigned stream, unsigned ttl /* = 0 */, unsigned context /* = 0 */,
                                unsigned ppid /* = 0 */, abortFlag abort /* = KEEPALIVE */,
				switchAddressFlag switchAddr /* = KEEP_PRIMARY */)
{
	int ret = sctp_sendmsg( m_socket, data, length, NULL, 0, ppid, abort + switchAddr + SCTP_UNORDERED, stream, ttl, context);
	if( ret < 0)
		throw SocketException("SCTPSocket::send failed");
	else if (ret < length)
		throw SocketException("Interpreted SCTP false, sent a packet fragement");
	return ret;
}

int SCTPSocket::receive( void* data, int maxLen, unsigned& stream)
{
	struct sctp_sndrcvinfo info;
	int ret;
	if( (ret = sctp_recvmsg( m_socket, data, maxLen, 0, 0, &info, 0)) <= 0)
		throw SocketException("SCTPSocket::receive failed");
	stream = info.sinfo_stream;
	return ret;
}

int SCTPSocket::receive( void* data, int maxLen, unsigned& stream, receiveFlag& flag)
{
	struct sctp_sndrcvinfo info;
	int ret;
	if( (ret = sctp_recvmsg( m_socket, data, maxLen, 0, 0, &info, 0)) <= 0)
		throw SocketException("SCTPSocket::receive failed");
	stream = info.sinfo_stream;
	// flag = info.sinfo_flags;
	return ret;
}

int SCTPSocket::timedReceive( void* data, int maxLen, unsigned& stream, unsigned timeout)
{
	struct pollfd poll;
	poll.fd = m_socket;
	poll.events = POLLIN | POLLPRI | POLLRDHUP;

	int ret = ::poll( &poll, 1, timeout);
	if( poll.revents & POLLRDHUP)
	{
		m_peerDisconnected = true;
		return 0;
	}

	if( ret == 0)
		return 0;
	if( ret < 0)
		throw SocketException("SCTPSocket::receive failed (poll)");

	struct sctp_sndrcvinfo info;
	if( (ret = sctp_recvmsg( m_socket, data, maxLen, 0, 0, &info, 0)) <= 0)
		throw SocketException("SCTPSocket::receive failed (receive)");
	stream = info.sinfo_stream;
	return ret;
}

int SCTPSocket::timedReceive( void* data, int maxLen, unsigned& stream, receiveFlag& flag, unsigned timeout)
{
	struct pollfd poll;
	poll.fd = m_socket;
	poll.events = POLLIN | POLLPRI | POLLRDHUP;

	int ret = ::poll( &poll, 1, timeout);
	if( poll.events & POLLRDHUP)
	{
		m_peerDisconnected = true;
		return 0;
	}

	if( ret == 0)
		return 0;
	if( ret < 0)
		throw SocketException("SCTPSocket::receive failed (poll)");
	struct sctp_sndrcvinfo info;
	if( (ret = sctp_recvmsg( m_socket, data, maxLen, 0, 0, &info, 0)) <= 0)
		throw SocketException("SCTPSocket::receive failed (receive)");
	stream = info.sinfo_stream;
	// flag = info.sinfo_flags;
	return ret;
}

void SCTPSocket::listen( int backlog /* = 0 */)
{
	::listen( m_socket, backlog);
}

SCTPSocket::Handle SCTPSocket::accept()
{
	int ret;
	if( (ret = ::accept( m_socket, 0, 0)) <= 0)
		throw SocketException("TCPSocket::accept failed");
	return Handle(ret);
}

void SCTPSocket::setInitValues( unsigned numOutStreams, unsigned maxInStreams, unsigned maxAttempts, unsigned maxInitTimeout)
{
	struct sctp_initmsg init;
	std::memset( &init, 0, sizeof(init));
	init.sinit_num_ostreams = numOutStreams;
	init.sinit_max_instreams = maxInStreams;
	init.sinit_max_attempts = maxAttempts;
	init.sinit_max_init_timeo = maxInitTimeout;
	::setsockopt( m_socket, IPPROTO_SCTP, SCTP_INITMSG, &init, sizeof(init));
}
