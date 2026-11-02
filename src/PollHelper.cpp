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
			return true;
		default:
			return false;
	}
}

}; // namespace

using namespace NET;

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
