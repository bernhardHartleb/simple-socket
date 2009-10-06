#ifndef NET_UnixSocket_h__
#define NET_UnixSocket_h__

#include "SimpleSocket.h"

struct sockaddr_un;

namespace NET
{
	//! Unix Domain Socket class
	class UnixSocket : public SimpleSocket
	{
	public:
		/*!
		 * Establish a socket connection with the given socket
		 * \param foreignPath the path of the remote socket
		 * \exception SocketException thrown if unable to establish connection
		 */
		void connect( const std::string& foreignPath);

		/*!
		 * Unset the socket path
		 * \exception SocketException thrown if unable to disconnect socket
		 */
		void disconnect();

		/*!
		 * Set the local path to the specified path
		 * \param localPath specifies where the socket should be bound
		 * \exception SocketException thrown if setting local path fails
		 */
		void bind( const std::string& localPath);

		/*!
		 * Get the local path
		 * \return local path of socket
		 * \exception SocketException thrown if fetch fails
		 */
		std::string getLocalPath() const;

		/*!
		 * Get the foreign path
		 * \return foreign path
		 * \exception SocketException thrown if fetch fails
		 */
		std::string getForeignPath() const;

		/*!
		 * Check the given path for size and other constraints
		 * \param path the path to check
		 * \return true if path is valid, false otherwise
		 */
		static bool isValidPath( const std::string& path);

	protected:
		//! allows a subclass to create new socket
		UnixSocket( int type, int protocol);

		//! extracts a path string from the socket address structure
		static std::string extractPath( const sockaddr_un& addr, socklen_t len);
	};

} // namespace NET

#endif // NET_UnixSocket_h__
