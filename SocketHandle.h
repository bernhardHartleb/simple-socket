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
	 * // socket is a TCPSocket listening for connections
	 * TCPSocket::Handle handle = socket.timedAccept(1000);
	 * if(!handle) {
	 *   std::cout << "no one tried to connect in time" << std::endl;
	 *   return;
	 * }
	 * TCPSocket acceptedSocket(handle);
	 * // use the socket
	 * \endcode
	 */
	template<class Socket>
	class SocketHandle
	{
	public:
		friend class TCPSocket;
		friend class SCTPSocket;

		//! socket type that was provided as template argument
		typedef Socket socket_type;

		//! constructs an invalid socket handle
		SocketHandle() : m_sockfd(-1) {}

		//! copy constructor using move semantics
		SocketHandle( SocketHandle& other)
		: m_sockfd( other.release()) {}

		//! \cond internal
		SocketHandle( SocketHandle_Ref<Socket> other)
		: m_sockfd( other.sockfd) {}

		operator SocketHandle_Ref<Socket>()
		{ return SocketHandle_Ref<Socket>( release()); }
		//! \endcond

		//! assignment operator using move semantics
		SocketHandle&
		operator=( SocketHandle& other)
		{
			reset( other.release());
			return *this;
		}

		//! \cond internal
		SocketHandle&
		operator=( SocketHandle_Ref<Socket> other)
		{
			reset( other.sockfd);
			return *this;
		}
		//! \endcond

		~SocketHandle() { reset(); }

		//! returns whether the socket handle is valid
		/*!
		 * Checks whether a socket can be constructed from the handle or not.
		 * If accept() returned an invalid handle (e.g. timed out timedAccept()
		 * call) this should be used to verify the handle. Trying to construct
		 * a socket from an invalid handle will raise a SocketException.
		 */
		operator bool() const { return m_sockfd >= 0; }

	private:
		//! constructor for sockets using this handle
		explicit SocketHandle( int sockfd) : m_sockfd(sockfd) {}

		/*!
		 * Releases ownership of the socket file descriptor.
		 * It also transfers the obligation to close the handle.
		 */
		int release()
		{
			int tmp = m_sockfd;
			m_sockfd = -1;
			return tmp;
		}

		//! resets the handle to an other socket file descriptor
		/*!
		 * If the new file descriptor differs from the old one,
		 * the previously held socket descriptor is closed.
		 */
		void reset( int fd = -1)
		{
			if( fd != m_sockfd)
			{
				TEMP_FAILURE_RETRY (::close(m_sockfd));
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
	//! \endcond

} // namespace NET

#endif // NET_SocketHandle_h__
