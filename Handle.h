#ifndef NET_SocketHandle_h__
#define NET_SocketHandle_h__

#include <unistd.h>
#include "TempFailure.h"

namespace NET
{
	template<class>
	class SocketHandle_Ref;

	class TCPSocket;
	class SCTPSocket;

	//! A simple class to provide strict ownership of socket handles.
	/*!
	 * SocketHandle is used to return accepted connections to the user, and
	 * not expose the socket file descriptor. It also ensures all an accepted
	 * socket is closed in the case no new socket is created with this handle.
	 *
	 * SocketHandle implements move semantics, so only one SocketHandle can be
	 * active for the same connection at any given moment. If SocketHandle is
	 * copied/passed to a function, etc. the source/passed in SocketHandle is
	 * made invalid. The SocketHandle also gets invalid if an socket is
	 * constructed from the handle.
	 *
	 * After a SocketHandle gets returned from an accept() call, the
	 * SocketHandle has to be checked wheter it is valid or not.
	 *
	 * Usage example:
	 * \code
	 * // socket is a TCPSocket listening on connections
	 * TCPSocket::Handle handle = socket.timedAccept(1000);
	 * if( !handle) {
	 *   std::cout << "no one tried to connect in time" << std::endl;
	 *   return;
	 * }
	 * TCPSocket acceptedSocket(handle);
	 * ... do something with the socket
	 * \endcode
	 */
	template<class Socket>
	class SocketHandle
	{
	public:
		friend class TCPSocket;
		friend class SCTPSocket;

		//! The Type that was provided as template argument
		typedef Socket socket_type;

		//! constructs an invalid socket handle
		SocketHandle() : m_sockfd(0) {}

		SocketHandle( SocketHandle& other)
		: m_sockfd( other.release()) {}

		SocketHandle( SocketHandle_Ref<Socket> other)
		: m_sockfd( other.sockfd) {}
		
		//! \cond internal
		operator SocketHandle_Ref<Socket>()
		{ return SocketHandle_Ref<Socket>( release()); }
		//! \endcond

		SocketHandle&
		operator=( SocketHandle& other)
		{
			reset( other.release());
			return *this;
		}

		~SocketHandle() { reset(); }

		//! returns wheter the socket is valid or not
		/*!
		 * Checks wheter a socket can be constructed from the handle or not.
		 * If accept() returned an invalid socket (e.g. timed out timedAccept()
		 * call) this should be used to verify the handle. Trying to construct
		 * a socket from an invalid handle will raise an exception.
		 *
		 */
		operator bool() const { return m_sockfd > 0; }

	private:
		//!
		explicit SocketHandle( int sockfd) : m_sockfd(sockfd) {}

		//! transfers the socket to an other entity
		/*!
		 * This also transfers the obligation to close the handle to the other
		 * entity
		 */
		int release()
		{
			int tmp = m_sockfd;
			m_sockfd = 0;
			return tmp;
		}

		//! resets the handle to an other socket file descriptor
		/*!
		 * If the handle is reset to a different file descriptor than itself,
		 * handle closes the previously held socket descriptor.
		 */
		void reset( int fd = 0)
		{
			if( fd != m_sockfd)
			{
				TEMP_FAILURE_RETRY( ::close(m_sockfd));
				m_sockfd = fd;
			}
		}

		int m_sockfd;
	};

	//! \cond internal
	
	//! A helper class to provide SocketHandle with reference semantics.
	template<class Socket>
	class SocketHandle_Ref
	{
		int sockfd;

		friend class SocketHandle<Socket>;
		explicit SocketHandle_Ref( int sockfd) : sockfd(sockfd) {}
	};
	/// \endcond

} // namespace NET

#endif // NET_SocketHandle_h__
