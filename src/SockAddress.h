#ifndef NET_SockAddress_h__
#define NET_SockAddress_h__

#include "SimpleSocket.h"

#include <cstdint>
#include <cstring>
#include <string>

#include <sys/socket.h>

namespace NET
{
	/*
	 * @internal
	 * sockaddr abstraction to handle IPv4/IPV6 and Unix paths
	 */
	class SockAddress
	{
	public:
		SockAddress() noexcept = default;

		// Numeric IPv4 or IPv6 address
		SockAddress( AddressFamily family, std::string_view address, unsigned short port);

		// Wildcard address: 0.0.0.0 or ::
		SockAddress( AddressFamily family, unsigned short port);

		// Unix filesystem pathname
		explicit SockAddress( std::string_view path);

		// Copy a native socket address
		explicit SockAddress( const sockaddr* address);

		sockaddr* data() noexcept;
		const sockaddr* data() const noexcept;
		socklen_t length() const noexcept;

		sa_family_t family() const noexcept;
		bool empty() const noexcept;

		// For getsockname(), getpeername(), recvfrom(), etc.
		// Resets internal state to accept new data from syscall
		socklen_t* lengthPtr() noexcept;

		// Valid for AF_INET / AF_INET6
		unsigned short port() const noexcept;

		std::string address() const;

		// Valid for AF_INET6. Returns 0 for other families or no scope.
		uint32_t scopeId() const noexcept;

		static sa_family_t nativeFamily(AddressFamily family) noexcept;

	private:
		template<typename T>
		void store(const T& address, socklen_t length) noexcept
		{
			static_assert(sizeof(T) <= sizeof(sockaddr_storage));
			static_assert(alignof(T) <= alignof(sockaddr_storage));

			m_storage = {};
			std::memcpy( &m_storage, &address, sizeof(address));
			m_length = length;
		}

		sockaddr_storage m_storage{};
		socklen_t m_length = sizeof(m_storage);
	};

} // namespace NET

#endif // NET_SockAddress_h__
