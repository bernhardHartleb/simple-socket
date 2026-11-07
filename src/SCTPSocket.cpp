#include "SCTPSocket.h"
#include "SockAddress.h"
#include "PollHelper.h"

using namespace NET;

namespace {

struct sctp_status getStatus( int socket)
{
	struct sctp_status status;
	socklen_t size = sizeof(status);
	if( ::getsockopt( socket, IPPROTO_SCTP, SCTP_STATUS, &status, &size) < 0)
		throw SocketException("Reading SCTP status failed (getsockopt)");
	return status;
}

} // namespace

SCTPSocket::SCTPSocket( uint16_t numOutStreams /* = 10 */,
                        uint16_t maxInStreams /* = 65535 */,
                        uint16_t maxAttempts /* = 4 */,
                        uint16_t maxInitTimeout /* = 0 */)
: InternetSocket( IPv4, SOCK_STREAM, IPPROTO_SCTP)
{
	setInitValues( numOutStreams, maxInStreams, maxAttempts, maxInitTimeout);
}

int SCTPSocket::bind( const std::vector<std::string>& localAddresses, unsigned short localPort /* = 0 */)
{
	std::vector<sockaddr_in> dest;
	dest.reserve(localAddresses.size());

	for( const auto& address : localAddresses)
	{
		SockAddress addr( AddressFamily::IPv4, address, localPort);
		sockaddr_in native{};
		std::memcpy( &native, addr.data(), sizeof(native));
		dest.push_back(native);
	}

	int ret = sctp_bindx( m_socket, reinterpret_cast<sockaddr*>(dest.data()), static_cast<int>(dest.size()), SCTP_BINDX_ADD_ADDR);
	if(ret < 0)
		throw SocketException("Set of local address and port failed (sctp_bindx)");
	return ret;
}

int SCTPSocket::connect( const std::vector<std::string>& foreignAddresses, unsigned short foreignPort /* = 0 */)
{
	std::vector<sockaddr_in> dest;
	dest.reserve(foreignAddresses.size());

	for( const auto& address : foreignAddresses)
	{
		SockAddress addr( AddressFamily::IPv4, address, foreignPort);
		sockaddr_in native{};
		std::memcpy( &native, addr.data(), sizeof(native));
		dest.push_back(native);
	}

	// TODO maybe save connection id for later use
	int ret = sctp_connectx( m_socket, reinterpret_cast<sockaddr*>(dest.data()), static_cast<int>(dest.size()), 0);
	if(ret < 0)
		throw SocketException("Connect failed (sctp_connectx)");

	return ret;
}

int SCTPSocket::state() const
{
	return getStatus(m_socket).sstat_state;
}

int SCTPSocket::notAckedData() const
{
	return getStatus(m_socket).sstat_unackdata;
}

int SCTPSocket::pendingData() const
{
	return getStatus(m_socket).sstat_penddata;
}

unsigned SCTPSocket::inStreams() const
{
	return getStatus(m_socket).sstat_instrms;
}

unsigned SCTPSocket::outStreams() const
{
	return getStatus(m_socket).sstat_outstrms;
}

unsigned SCTPSocket::fragmentationPoint() const
{
	return getStatus(m_socket).sstat_fragmentation_point;
}

std::string SCTPSocket::primaryAddress() const
{
	return std::string(); // TODO
}

IOResult SCTPSocket::send( const void* data, size_t length, uint16_t stream, unsigned ttl /* = 0 */, unsigned context /* = 0 */,
	unsigned ppid /* = 0 */, abortFlag abort /* = KEEPALIVE */, switchAddressFlag switchAddr /* = KEEP_PRIMARY */)
{
	int ret = sctp_sendmsg( m_socket, data, length, nullptr, 0, ppid, abort + switchAddr, stream, ttl, context);
	if( ret > 0)
		return { IOStatus::Success, static_cast<size_t>(ret) };

	auto status = detail::classifyIoError( errno, m_connectionOriented);
	if( status) return IOResult{ *status, 0 };

	throw SocketException("Send failed (sctp_sendmsg)");
}

IOResult SCTPSocket::sendUnordered( const void* data, size_t length, uint16_t stream, unsigned ttl /* = 0 */, unsigned context /* = 0 */,
	unsigned ppid /* = 0 */, abortFlag abort /* = KEEPALIVE */, switchAddressFlag switchAddr /* = KEEP_PRIMARY */)
{
	int ret = sctp_sendmsg( m_socket, data, length, nullptr, 0, ppid, abort + switchAddr + SCTP_UNORDERED, stream, ttl, context);
	if( ret > 0)
		return { IOStatus::Success, static_cast<size_t>(ret) };

	auto status = detail::classifyIoError( errno, m_connectionOriented);
	if( status) return IOResult{ *status, 0 };

	throw SocketException("Send failed (sctp_sendmsg)");
}

IOResult SCTPSocket::receive( void* data, size_t maxLen, uint16_t& stream)
{
	struct sctp_sndrcvinfo info;
	int ret = sctp_recvmsg( m_socket, data, maxLen, 0, 0, &info, 0);
	if( ret < 0)
	{
		auto status = detail::classifyIoError( errno, m_connectionOriented);
		if( status) return IOResult{ *status, 0 };

		throw SocketException("Receive failed (sctp_recvmsg)");
	}
	// for SCTP recv() == 0 means EOF
	if( ret == 0)
		return { IOStatus::PeerDisconnected, 0 };

	stream = info.sinfo_stream;
	return { IOStatus::Success, static_cast<size_t>(ret) };
}

IOResult SCTPSocket::receive( void* data, size_t maxLen, uint16_t& stream, receiveFlag& flag)
{
	struct sctp_sndrcvinfo info;
	int ret = sctp_recvmsg( m_socket, data, maxLen, 0, 0, &info, 0);
	if( ret < 0)
	{
		auto status = detail::classifyIoError( errno, m_connectionOriented);
		if( status) return IOResult{ *status, 0 };

		throw SocketException("Receive failed (sctp_recvmsg)");
	}
	// for SCTP recv() == 0 means EOF
	if( ret == 0)
		return { IOStatus::PeerDisconnected, 0 };

	stream = info.sinfo_stream;
	flag = static_cast<receiveFlag>(info.sinfo_flags);
	return { IOStatus::Success, static_cast<size_t>(ret) };
}

IOResult SCTPSocket::timedReceive( void* data, size_t maxLen, uint16_t& stream, int timeout)
{
	return detail::timedReceiveImpl( m_socket, m_connectionOriented, timeout,
		[&] { return receive( data, maxLen, stream); });
}

IOResult SCTPSocket::timedReceive( void* data, size_t maxLen, uint16_t& stream, receiveFlag& flag, int timeout)
{
	return detail::timedReceiveImpl( m_socket, m_connectionOriented, timeout,
		[&] { return receive( data, maxLen, stream, flag); });
}

std::optional<SCTPSocket> SCTPSocket::accept() const
{
	int ret = SimpleSocket::accept();
	if( ret >= 0)
		return std::optional<SCTPSocket>{ SCTPSocket( AdoptSocketTag{}, ret, SOCK_STREAM)};
	else
		return std::nullopt;
}

std::optional<SCTPSocket> SCTPSocket::timedAccept( int timeout) const
{
	int ret = SimpleSocket::timedAccept(timeout);
	if( ret >= 0)
		return std::optional<SCTPSocket>{ SCTPSocket( AdoptSocketTag{}, ret, SOCK_STREAM)};
	else
		return std::nullopt;
}

void SCTPSocket::setInitValues( uint16_t numOutStreams, uint16_t maxInStreams, uint16_t maxAttempts, uint16_t maxInitTimeout)
{
	struct sctp_initmsg init;
	init.sinit_num_ostreams = numOutStreams;
	init.sinit_max_instreams = maxInStreams;
	init.sinit_max_attempts = maxAttempts;
	init.sinit_max_init_timeo = maxInitTimeout;

	if( ::setsockopt( m_socket, IPPROTO_SCTP, SCTP_INITMSG, &init, sizeof(init)) < 0)
		throw SocketException("SCTPSocket construction failed (setsockopt)");
}
