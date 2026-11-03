#include "PollHelper.h"

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
		case ECONNREFUSED:
			return ConnectResult::Refused;
		case ECONNABORTED:
		case ECONNRESET:
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
