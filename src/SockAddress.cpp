#include "SockAddress.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/un.h>

using namespace NET;

namespace {

void copyNumericAddress( std::string_view address, char (&buffer)[INET6_ADDRSTRLEN])
{
	// inet_pton() requires a NUL-terminated string.
	// INET6_ADDRSTRLEN includes room for the terminating NUL of
	// the longest normal textual IPv6 representation.
	if( address.empty() ||
		address.size() >= sizeof(buffer) ||
		address.find('\0') != std::string_view::npos)
	{
		throw SocketException("Invalid IP address", false);
	}

	std::memcpy( buffer, address.data(), address.size());
	buffer[address.size()] = '\0';
}

void checkInetPtonResult( int result, const char* invalidMessage)
{
	if (result == 1) return;
	if (result == 0) throw SocketException( invalidMessage, false);
	// inet_pton() returned -1.
	// Unlike a malformed input string, this is a real errno-based
	// system error (normally EAFNOSUPPORT).
	throw SocketException("Address conversion failed (inet_pton)");
}

} // namespace

SockAddress::SockAddress( AddressFamily family, std::string_view address, unsigned short port)
{
	char text[INET6_ADDRSTRLEN];

	copyNumericAddress(address, text);

	switch (nativeFamily(family))
	{
		case AF_INET:
		{
			sockaddr_in addr{};
			addr.sin_family = AF_INET;
			addr.sin_port = htons(port);
			const int result = ::inet_pton(AF_INET, text, &addr.sin_addr);
			checkInetPtonResult(result, "Invalid IPv4 address");
			store(addr, sizeof(addr));
			break;
		}
		case AF_INET6:
		{
			sockaddr_in6 addr{};
			addr.sin6_family = AF_INET6;
			addr.sin6_port = htons(port);
			const int result = ::inet_pton(AF_INET6, text, &addr.sin6_addr);
			checkInetPtonResult(result, "Invalid IPv6 address");
			store(addr, sizeof(addr));
			break;
		}
		default:
			throw SocketException("Unsupported address family", false);
	}
}

SockAddress::SockAddress( AddressFamily family, unsigned short port)
{
	switch (nativeFamily(family))
	{
		case AF_INET:
		{
			sockaddr_in addr{};
			addr.sin_family = AF_INET;
			addr.sin_port = htons(port);
			addr.sin_addr.s_addr = htonl(INADDR_ANY);
			store( addr, sizeof(addr));
			break;
		}
		case AF_INET6:
		{
			sockaddr_in6 addr{};
			addr.sin6_family = AF_INET6;
			addr.sin6_port = htons(port);
			addr.sin6_addr = in6addr_any;
			store( addr, sizeof(addr));
			break;
		}
		default:
			throw SocketException("Unsupported address family", false);
	}
}

SockAddress::SockAddress( std::string_view path)
{
	sockaddr_un addr{};
	addr.sun_family = AF_UNIX;

	// This constructor represents filesystem Unix-domain addresses,
	// therefore embedded NUL characters are not meaningful here.
	if( path.empty() ||
		path.size() >= sizeof(addr.sun_path) ||
		path.find('\0') != std::string_view::npos)
	{
		throw SocketException("Invalid Unix socket path", false);
	}

	std::memcpy( addr.sun_path, path.data(), path.size());
	addr.sun_path[path.size()] = '\0';

	const auto length = static_cast<socklen_t>( offsetof(sockaddr_un, sun_path) + path.size() + 1);
	store( addr, length);
}

SockAddress::SockAddress( const sockaddr* address)
{
	if( address == nullptr)
		throw SocketException("Null socket address", false);

	socklen_t size = 0;
	switch( address->sa_family)
	{
		case AF_INET:
			size = static_cast<socklen_t>(sizeof(sockaddr_in));
			break;
		case AF_INET6:
			size = static_cast<socklen_t>(sizeof(sockaddr_in6));
			break;
		default:
			throw SocketException("Unsupported address family", false);
	}
	m_storage = {};
	std::memcpy( &m_storage, address, size);
	m_length = size;
}

sockaddr* SockAddress::data() noexcept
{
	return reinterpret_cast<sockaddr*>(&m_storage);
}

const sockaddr* SockAddress::data() const noexcept
{
	return reinterpret_cast<const sockaddr*>(&m_storage);
}

socklen_t SockAddress::length() const noexcept
{
	return m_length;
}

sa_family_t SockAddress::family() const noexcept
{
	return m_storage.ss_family;
}

bool SockAddress::empty() const noexcept
{
	return m_storage.ss_family == AF_UNSPEC;
}

socklen_t* SockAddress::lengthPtr() noexcept
{
	m_storage = {};
	m_length = sizeof(m_storage);
	return &m_length;
}

unsigned short SockAddress::port() const noexcept
{
	switch (family())
	{
		case AF_INET:
		{
			sockaddr_in addr{};
			std::memcpy( &addr, &m_storage, sizeof(addr));
			return ntohs(addr.sin_port);
		}
		case AF_INET6:
		{
			sockaddr_in6 addr{};
			std::memcpy( &addr, &m_storage, sizeof(addr));
			return ntohs(addr.sin6_port);
		}
		default:
			return 0;
	}
}

std::string SockAddress::address() const
{
	char buffer[INET6_ADDRSTRLEN];

	switch (family())
	{
		case AF_INET:
		{
			sockaddr_in addr{};
			std::memcpy(&addr, &m_storage, sizeof(addr));

			if (::inet_ntop( AF_INET, &addr.sin_addr, buffer, sizeof(buffer)) == nullptr)
				throw SocketException("Address conversion failed (inet_ntop)");

			return buffer;
		}
		case AF_INET6:
		{
			sockaddr_in6 addr{};
			std::memcpy(&addr, &m_storage, sizeof(addr));

			if (::inet_ntop( AF_INET6, &addr.sin6_addr, buffer, sizeof(buffer)) == nullptr)
				throw SocketException("Address conversion failed (inet_ntop)");

			return buffer;
		}
		case AF_UNIX:
		{
			sockaddr_un addr{};
			std::memcpy(&addr, &m_storage, sizeof(addr));

			constexpr std::size_t offset = offsetof(sockaddr_un, sun_path);
			if (m_length <= offset) return {};

			std::size_t length = static_cast<std::size_t>(m_length) - offset;

			if (length > sizeof(addr.sun_path))
				length = sizeof(addr.sun_path);

			// Filesystem Unix socket addresses normally include
			// the terminating NUL in m_length.
			if (length != 0 && addr.sun_path[length - 1] == '\0')
				--length;

			return std::string(addr.sun_path, length);
		}
		case AF_UNSPEC:
			return {};
		default:
			throw SocketException("Unsupported address family", false);
	}
}

uint32_t SockAddress::scopeId() const noexcept
{
	if( family() != AF_INET6) return 0;

	sockaddr_in6 addr{};
	std::memcpy(&addr, &m_storage, sizeof(addr));
	return addr.sin6_scope_id;
}

sa_family_t SockAddress::nativeFamily( AddressFamily family) noexcept
{
	switch (family)
	{
	case AddressFamily::IPv4: return AF_INET;
	case AddressFamily::IPv6: return AF_INET6;
	}
	return AF_UNSPEC;
}
