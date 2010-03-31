#include "SCTPSocket.h"
#include "TempFailure.h"

#include <vector>
#include <netinet/in.h>
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

SCTPSocket::SCTPSocket( Handle handle)
: InternetSocket( handle.release() )
{}

int SCTPSocket::bind( const std::vector<std::string>& localAddresses, unsigned short localPort /* = 0 */)
{
	size_t size = localAddresses.size();
	std::vector<sockaddr_in> dest(size);

	for( size_t i = 0; i < size; ++i)
		fillAddress( localAddresses[i], localPort, dest[i]);

	int ret = sctp_bindx( m_socket, reinterpret_cast<sockaddr*>(dest.data()), size, SCTP_BINDX_ADD_ADDR);
	if(ret < 0)
		throw SocketException("Set of local address and port failed (sctp_bindx)");
	return ret;
}

int SCTPSocket::connect( const std::vector<std::string>& foreignAddresses, unsigned short foreignPort /* = 0 */)
{
	size_t size = foreignAddresses.size();
	std::vector<sockaddr_in> dest(size);

	for( size_t i = 0; i < size; ++i)
		fillAddress( foreignAddresses[i], foreignPort, dest[i]);

	int ret = sctp_connectx( m_socket, reinterpret_cast<sockaddr*>(dest.data()), size);
	if(ret < 0)
		throw SocketException("Connect failed (sctp_connectx)");

	m_peerDisconnected = false;
	return ret;
}

int SCTPSocket::state() const
{
	struct sctp_status status;
	socklen_t size = sizeof(status);
	if( getsockopt( m_socket, IPPROTO_SCTP, SCTP_STATUS, &status, &size) < 0)
		throw SocketException("SCTPSocket::state failed (getsockopt)");
	return status.sstat_state;
}

int SCTPSocket::notAckedData() const
{
	struct sctp_status status;
	socklen_t size = sizeof(status);
	if( getsockopt( m_socket, IPPROTO_SCTP, SCTP_STATUS, &status, &size) < 0)
		throw SocketException("SCTPSocket::notAckedData failed (getsockopt)");
	return status.sstat_unackdata;
}

int SCTPSocket::pendingData() const
{
	struct sctp_status status;
	socklen_t size = sizeof(status);
	if( getsockopt( m_socket, IPPROTO_SCTP, SCTP_STATUS, &status, &size) < 0)
		throw SocketException("SCTPSocket::pendingData failed (getsockopt)");
	return status.sstat_penddata;
}

unsigned SCTPSocket::inStreams() const
{
	struct sctp_status status;
	socklen_t size = sizeof(status);
	if( getsockopt( m_socket, IPPROTO_SCTP, SCTP_STATUS, &status, &size) < 0)
		throw SocketException("SCTPSocket::inStreams failed (getsockopt)");
	return status.sstat_instrms;
}

unsigned SCTPSocket::outStreams() const
{
	struct sctp_status status;
	socklen_t size = sizeof(status);
	if( getsockopt( m_socket, IPPROTO_SCTP, SCTP_STATUS, &status, &size) < 0)
		throw SocketException("SCTPSocket::outStreams failed (getsockopt)");
	return status.sstat_outstrms;
}

unsigned SCTPSocket::fragmentationPoint() const
{
	struct sctp_status status;
	socklen_t size = sizeof(status);
	if( getsockopt( m_socket, IPPROTO_SCTP, SCTP_STATUS, &status, &size) < 0)
		throw SocketException("SCTPSocket::fragmentationPoint failed (getsockopt)");
	return status.sstat_fragmentation_point;
}

std::string SCTPSocket::primaryAddress() const
{
	return std::string(); // TODO
}

int SCTPSocket::send( const void* data, int length, unsigned stream, unsigned ttl /* = 0 */, unsigned context /* = 0 */,
                      unsigned ppid /* = 0 */, abortFlag abort /* = KEEPALIVE */, switchAddressFlag switchAddr /* = KEEP_PRIMARY */)
{
	int ret = sctp_sendmsg( m_socket, data, length, NULL, 0, ppid, abort + switchAddr, stream, ttl, context);
	if( ret < 0)
		throw SocketException("SCTPSocket::send failed (sctp_sendmsg)");
	else if (ret < length)
		throw SocketException("SCTP sent a fragemented packet!");
	return ret;
}

int SCTPSocket::sendUnordered( const void* data, int length, unsigned stream, unsigned ttl /* = 0 */, unsigned context /* = 0 */,
                               unsigned ppid /* = 0 */, abortFlag abort /* = KEEPALIVE */,
                               switchAddressFlag switchAddr /* = KEEP_PRIMARY */)
{
	int ret = sctp_sendmsg( m_socket, data, length, NULL, 0, ppid, abort + switchAddr + SCTP_UNORDERED, stream, ttl, context);
	if( ret < 0)
		throw SocketException("SCTPSocket::send failed (sctp_sendmsg)");
	else if (ret < length)
		throw SocketException("SCTP sent a fragemented packet!");
	return ret;
}

int SCTPSocket::receive( void* data, int maxLen, unsigned& stream)
{
	struct sctp_sndrcvinfo info;
	int ret;
	if( (ret = sctp_recvmsg( m_socket, data, maxLen, 0, 0, &info, 0)) <= 0)
		throw SocketException("SCTPSocket::receive failed (sctp_recvmsg)");
	stream = info.sinfo_stream;
	return ret;
}

int SCTPSocket::receive( void* data, int maxLen, unsigned& stream, receiveFlag& flag)
{
	struct sctp_sndrcvinfo info;
	int ret;
	if( (ret = sctp_recvmsg( m_socket, data, maxLen, 0, 0, &info, 0)) <= 0)
		throw SocketException("SCTPSocket::receive failed (sctp_recvmsg)");
	stream = info.sinfo_stream;
	flag = static_cast<receiveFlag>(info.sinfo_flags);
	return ret;
}

int SCTPSocket::timedReceive( void* data, int maxLen, unsigned& stream, unsigned timeout)
{
	struct pollfd poll;
	poll.fd = m_socket;
	poll.events = POLLIN | POLLPRI | POLLRDHUP;

	int ret = TEMP_FAILURE_RETRY (::poll( &poll, 1, timeout));

	if( ret == 0) return 0;
	if( ret < 0) throw SocketException("SCTPSocket::timedReceive failed (poll)");

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
	if( ret < 0) throw SocketException("SCTPSocket::timedReceive failed (poll)");

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
		throw SocketException("SCTPSocket::accept failed (accept)");
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
	init.sinit_num_ostreams = numOutStreams;
	init.sinit_max_instreams = maxInStreams;
	init.sinit_max_attempts = maxAttempts;
	init.sinit_max_init_timeo = maxInitTimeout;

	int ret = setsockopt( m_socket, IPPROTO_SCTP, SCTP_INITMSG, &init, sizeof(init));
	if( ret < 0)
		throw SocketException("SCTPSocket construction failed (setsockopt)");
}
