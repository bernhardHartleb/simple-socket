#ifndef NET_SocketHandle_h__
#define NET_SocketHandle_h__

#include <unistd.h>

namespace NET
{
	class TCPSocket;
	class SCTPSocket;

	//! A helper class to provide SocketHandle with reference semantics.
	template<class Socket>
	struct SocketHandle_Ref
	{
		int sockfd;

		explicit SocketHandle_Ref( int sockfd) : sockfd(sockfd) {}
	};

	//! A simple class to provide strict ownership of socket handles.
	template<class Socket>
	class SocketHandle
	{
	public:
		friend class TCPSocket;
		friend class SCTPSocket;

		typedef Socket socket_type;

		explicit SocketHandle() : m_sockfd(0) {}

		SocketHandle( SocketHandle& other)
		: m_sockfd( other.release()) {}

		SocketHandle( SocketHandle_Ref<Socket> other)
		: m_sockfd( other.sockfd) {}

		operator SocketHandle_Ref<Socket>()
		{ return SocketHandle_Ref<Socket>( release()); }

		SocketHandle&
		operator=( SocketHandle& other)
		{
			reset( other.release());
			return *this;
		}

		~SocketHandle() { reset(); }

		//!
		operator bool() const { return m_sockfd > 0; }

	private:
		//!
		explicit SocketHandle( int sockfd) : m_sockfd(sockfd) {}

		//!
		int release()
		{
			int tmp = m_sockfd;
			m_sockfd = 0;
			return tmp;
		}

		//!
		void reset( int fd = 0)
		{
			if( fd != m_sockfd)
			{
				::close(m_sockfd);
				m_sockfd = fd;
			}
		}

		int m_sockfd;
	};

} // namespace NET

#endif // NET_SocketHandle_h__
