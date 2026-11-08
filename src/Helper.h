#ifndef NET_Helper_h__
#define NET_Helper_h__

#include "SimpleSocket.h"

#include <optional>

namespace NET::detail
{
	//! return correct IOStatus for each un-excpetional failures
	std::optional<IOStatus> classifyIoError( int error, bool connectionOriented) noexcept;

	//! maps errno values into user visible connect result
	std::optional<ConnectResult> connectResultFromError( int error) noexcept;

	//! returns true when to retry accept() due to a transient failure
	bool isTransientAcceptError( int error) noexcept;

	//! check if external socket descriptor to fits the given parameters
	void validateSocket( int sockfd, int type, int protocol);


	//! switch socket to non-blocking while instance lives
	class NonBlockingGuard
	{
	public:
		explicit NonBlockingGuard(int fd);
		~NonBlockingGuard();

		NonBlockingGuard(const NonBlockingGuard&) = delete;
		NonBlockingGuard& operator=(const NonBlockingGuard&) = delete;

	private:
		int m_fd;
		int m_flags;
	};

} // namespace NET::detail

#endif // NET_Helper_h__
