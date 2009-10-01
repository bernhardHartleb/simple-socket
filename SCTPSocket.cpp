#include "SCTPSocket.h"
#include "TempFailure.h"
#include "SocketUtils.h"

#include <netinet/in.h>
#include <poll.h>
#include <cstring>

using namespace NET;

SCTPSocket::SCTPSocket( unsigned numOutStreams /* = 10 */,
                        unsigned maxInStreams /* = 65535 */,
                        unsigned maxAttempts /* = 4 */,
                        unsigned maxInitTimeout /* = 0 */)
: InternetSocket( STREAM, IPPROTO_SCTP)
{
	setInitValues( numOutStreams, maxInStreams, maxAttempts, maxInitTimeout);
}

SCTPSocket::SCTPSocket( Handle handle)
: InternetSocket( handle.release() )
{}

int SCTPSocket::bind( const std::vector<std::string>& localAddresses, unsigned short localPort /* = 0 */)
{
	sockaddr_in *dest = new sockaddr_in[localAddresses.size()];
	std::vector<std::string>::const_iterator it;
	std::vector<std::string>::const_iterator end;
	int i = 0;
	for( it = localAddresses.begin(), end = localAddresses.end(); it != end; ++it)
	{
		fillAddress( *it, localPort, dest[i++]);
	}
	int ret = sctp_bindx( m_socket, reinterpret_cast<sockaddr*>(dest), localAddresses.size(), SCTP_BINDX_ADD_ADDR);
	delete[] dest;
	if(ret < 0)
		throw SocketException("SCTPSocket::bind bindx failed");
	return ret;
}

int SCTPSocket::connect( const std::vector<std::string>& foreignAddresses, unsigned short foreignPort /* = 0 */)
{
	sockaddr_in *dest = new sockaddr_in[foreignAddresses.size()];
	std::vector<std::string>::const_iterator it;
	std::vector<std::string>::const_iterator end;
	int i = 0;
	for( it = foreignAddresses.begin(), end = foreignAddresses.end(); it != end; ++it)
	{
		fillAddress( *it, foreignPort, dest[i++]);
	}
	int ret = sctp_connectx( m_socket, reinterpret_cast<sockaddr*>(dest), foreignAddresses.size());
	delete[] dest;
	if(ret < 0)
		throw SocketException("SCTPSocket::SCTPSocket connect failed");
}

int SCTPSocket::state() const
{
	struct sctp_status status;
	socklen_t size = sizeof(status);
	if( getsockopt( m_socket, IPPROTO_SCTP, SCTP_STATUS, &status, &size) < 0)
		throw SocketException("SCTPSocket::state failed");
	return status.sstat_state;
}

int SCTPSocket::notAckedData() const
{
	struct sctp_status status;
	socklen_t size = sizeof(status);
	if( getsockopt( m_socket, IPPROTO_SCTP, SCTP_STATUS, &status, &size) < 0)
		throw SocketException("SCTPSocket::notAckedData failed");
	return status.sstat_unackdata;
}

int SCTPSocket::pendingData() const
{
	struct sctp_status status;
	socklen_t size = sizeof(status);
	if( getsockopt( m_socket, IPPROTO_SCTP, SCTP_STATUS, &status, &size) < 0)
		throw SocketException("SCTPSocket::pendingData failed");
	return status.sstat_penddata;
}

unsigned SCTPSocket::inStreams() const
{
	struct sctp_status status;
	socklen_t size = sizeof(status);
	if( getsockopt( m_socket, IPPROTO_SCTP, SCTP_STATUS, &status, &size) < 0)
		throw SocketException("SCTPSocket::inStreams failed");
	return status.sstat_instrms;
}

unsigned SCTPSocket::outStreams() const
{
	struct sctp_status status;
	socklen_t size = sizeof(status);
	if( getsockopt( m_socket, IPPROTO_SCTP, SCTP_STATUS, &status, &size) < 0)
		throw SocketException("SCTPSocket::outStreams failed");
	return status.sstat_outstrms;
}

unsigned SCTPSocket::fragmentationPoint() const
{
	struct sctp_status status;
	socklen_t size = sizeof(status);
	if( getsockopt( m_socket, IPPROTO_SCTP, SCTP_STATUS, &status, &size) < 0)
		throw SocketException("SCTPSocket::fragmentationPoint failed");
	return status.sstat_fragmentation_point;
}

std::string SCTPSocket::primaryAddress() const
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

int SCTPSocket::sendUnordered( const void* data, int length, unsigned stream, unsigned ttl /* = 0 */, unsigned context /* = 0 */,
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

	int ret = TEMP_FAILURE_RETRY (::poll( &poll, 1, timeout));

	if( ret == 0) return 0;
	if( ret < 0) throw SocketException("SCTPSocket::receive failed (poll)");

	if( poll.revents & POLLRDHUP)
		m_peerDisconnected = true;

	if( poll.revents & POLLIN || poll.revents & POLLPRI)
		return receive( data, maxLen, stream);

	return 0;
}

int SCTPSocket::timedReceive( void* data, int maxLen, unsigned& stream, receiveFlag& flag, unsigned timeout)
{
	struct pollfd poll;
	poll.fd = m_socket;
	poll.events = POLLIN | POLLPRI | POLLRDHUP;

	int ret = TEMP_FAILURE_RETRY (::poll( &poll, 1, timeout));

	if( ret == 0) return 0;
	if( ret < 0) throw SocketException("SCTPSocket::receive failed (poll)");

	if( poll.revents & POLLRDHUP)
		m_peerDisconnected = true;

	if( poll.revents & POLLIN || poll.revents & POLLPRI)
		return receive( data, maxLen, stream, flag);

	return 0;
}

void SCTPSocket::listen( int backlog /* = 5 */)
{
	int ret = ::listen( m_socket, backlog);
	if( ret < 0)
		throw SocketException("listen failed, most likely another socket is already listening on the same port");
}

SCTPSocket::Handle SCTPSocket::accept() const
{
	int ret = ::accept( m_socket, 0, 0);
	if( ret <= 0)
		throw SocketException("SCTPSocket::accept failed");
	return Handle(ret);
}

SCTPSocket::Handle SCTPSocket::timedAccept( unsigned timeout) const
{
	struct pollfd poll;
	poll.fd = m_socket;
	poll.events = POLLIN;

	int ret = TEMP_FAILURE_RETRY (::poll( &poll, 1, timeout));

	if( ret == 0) return Handle(0);
	if( ret < 0) throw SocketException("SCTPSocket::timedAccept failed (poll)");

	ret = ::accept( m_socket, 0, 0);
	if( ret <= 0)
		throw SocketException("SCTPSocket::timedAccept failed (accept)");
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
	setsockopt( m_socket, IPPROTO_SCTP, SCTP_INITMSG, &init, sizeof(init));
}
