#ifndef NET_SocketHandle_h__
#define NET_SocketHandle_h__

#include <unistd.h>

namespace NET
{
	class TCPSocket;
	class SCTPSocket;

	//! A simple class to provide strict ownership of socket handles.
	/*!
	 * SocketHandle is used to return accepted connections to the user, and
	 * not expose the socket file descriptor. It also ensures all an accepted
	 * socket is closed in the case no new socket is created with this handle.
	 *
	 * SocketHandle is move-only. Moving transfers ownership of the socket file
	 * descriptor and leaves the source handle invalid. Copying is not allowed.
	 * Constructing a socket from a handle also transfers ownership.
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
	 * TCPSocket acceptedSocket(std::move(handle));
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
		SocketHandle() noexcept : m_sockfd(-1) {}

		SocketHandle( const SocketHandle&) = delete;
		SocketHandle& operator=( const SocketHandle&) = delete;

		//! move constructor: transfers ownership from other
		SocketHandle( SocketHandle&& other) noexcept : m_sockfd(other.release()) {}

		//! move assignment: closes the currently owned descriptor first
		SocketHandle& operator=( SocketHandle&& other) noexcept
		{
			if( this != &other)
				reset(other.release());
			return *this;
		}

		~SocketHandle() noexcept { reset(); }

		//! returns whether the socket handle is valid
		/*!
		 * Checks whether a socket can be constructed from the handle or not.
		 * If accept() returned an invalid handle (e.g. timed out timedAccept()
		 * call) this should be used to verify the handle. Trying to construct
		 * a socket from an invalid handle will raise a SocketException.
		 * \exception SocketException if trying to use an invalid SocketHandle to create socket
		 */
		explicit operator bool() const noexcept { return m_sockfd >= 0; }

	private:
		//! constructor for sockets using this handle
		explicit SocketHandle( int sockfd) noexcept : m_sockfd(sockfd) {}

		/*!
		 * Releases ownership of the socket file descriptor.
		 * It also transfers the obligation to close the handle.
		 */
		int release() noexcept
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
		void reset( int fd = -1) noexcept
		{
			if( fd != m_sockfd)
			{
				if( m_sockfd >= 0) ::close(m_sockfd);
				m_sockfd = fd;
			}
		}

		int m_sockfd;
	};

} // namespace NET

#endif // NET_SocketHandle_h__
