#include "Helper.h"

#include <sys/socket.h>
#include <fcntl.h>
#include <cerrno>

namespace {

bool isConnectionLossError( int error) noexcept
{
	switch(error)
	{
		case ECONNRESET:   // the connection died
		case ECONNABORTED: // connection was aborted
		case ECONNREFUSED: // configured peer rejected it
		case EPIPE:        // connection can no longer send
		case ETIMEDOUT:    // connection died by timeout
			return true;
		default:
			return false;
	}
}

bool isNonFatalDatagramError( int error) noexcept
{
	switch(error)
	{
		case ECONNREFUSED:
		case EHOSTUNREACH:
		case ENETUNREACH:
		//case ENETDOWN: // leaving this out means throw when network is down
			return true;
		default:
			return false;
	}
}

}; // namespace

using namespace NET;

bool detail::isTransientAcceptError( int error) noexcept
{
	switch(error)
	{
		case ECONNABORTED:
		// Linux accept(2) explicitly recommends treating these
		// pending network errors like EAGAIN and retrying.
		case ENETDOWN:
		case EPROTO:
		case ENOPROTOOPT:
		case EHOSTDOWN:
		case ENONET:
		case EHOSTUNREACH:
		case EOPNOTSUPP:
		case ENETUNREACH:
		// Linux may also propagate this from the pending connection.
		case ETIMEDOUT:
			return true;
		default:
			return false;
	}
}

std::optional<IOStatus> detail::classifyIoError( int error, bool connectionOriented) noexcept
{
	if( error == EAGAIN || error == EWOULDBLOCK)
		return IOStatus::WouldBlock;

	if( connectionOriented && isConnectionLossError(error))
		return IOStatus::PeerDisconnected;

	if( !connectionOriented && isNonFatalDatagramError(error))
		return IOStatus::DestinationUnavailable;

	return std::nullopt;
}

std::optional<ConnectResult> detail::connectResultFromError( int error) noexcept
{
	switch(error)
	{
		case EINPROGRESS:
		case EALREADY:
		case EINTR:
			return ConnectResult::InProgress;
		case ECONNREFUSED:
			return ConnectResult::Refused;
		case ECONNABORTED:
		case ECONNRESET:
		case EAGAIN:
			return ConnectResult::Aborted;
		case ETIMEDOUT:
			return ConnectResult::Timeout;
		case EHOSTUNREACH:
		case ENETUNREACH:
			return ConnectResult::Unreachable;
		default:
			return std::nullopt;
	}
}

void detail::validateSocket( int sockfd, int type_req, int protocol_req)
{
	if( sockfd < 0)
		throw SocketException("Invalid native handle", false);

	int type = 0;
	socklen_t len = sizeof(type);
	if( ::getsockopt( sockfd, SOL_SOCKET, SO_TYPE, &type, &len) < 0)
		throw SocketException("Unable to read native socket type (getsockopt)");
	if( type != type_req)
		throw SocketException("Native handle is the wrong socket type", false);

#ifdef SO_PROTOCOL
	int protocol = 0;
	len = sizeof(protocol);
	if( ::getsockopt( sockfd, SOL_SOCKET, SO_PROTOCOL, &protocol, &len) < 0)
		throw SocketException("Unable to read native socket protocol (getsockopt)");
	if( protocol != protocol_req)
		throw SocketException("Native handle uses wrong protocol type", false);
#endif
}

detail::NonBlockingGuard::NonBlockingGuard(int fd) : m_fd(fd)
{
	m_flags = ::fcntl( m_fd, F_GETFL, 0);
	if( m_flags < 0)
		throw SocketException("Unable to get file status (fcntl)");
	if( ::fcntl( m_fd, F_SETFL, m_flags | O_NONBLOCK) < 0)
		throw SocketException("Unable to set non-blocking mode (fcntl)");
}

detail::NonBlockingGuard::~NonBlockingGuard()
{
	const int savedErrno = errno;
	::fcntl( m_fd, F_SETFL, m_flags);
	errno = savedErrno;
}
