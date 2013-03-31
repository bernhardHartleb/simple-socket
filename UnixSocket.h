#ifndef NET_UnixSocket_h__
#define NET_UnixSocket_h__

#include "SimpleSocket.h"

struct sockaddr_un;

namespace NET
{
	//! Unix domain socket class
	/*!
	 *	Base class for sockets identified by files in the filessystem.
	 */
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
		 * Set the local path to the specified path
		 * \param localPath specifies where the socket should be bound
		 * \exception SocketException thrown if setting local path fails
		 */
		void bind( const std::string& localPath);

		/*!
		 * Get the local path (after binding the socket)
		 * \return local path of socket
		 * \exception SocketException thrown if fetch fails
		 */
		std::string getLocalPath() const;

		/*!
		 * Get the foreign path. Call connect() before using this function.
		 * \return foreign path
		 * \exception SocketException thrown if fetch fails
		 */
		std::string getForeignPath() const;

	protected:
		//! allows a subclass to create new socket
		UnixSocket( int type, int protocol);

		/*!
		 * Fill an address structure with the given path.
		 * If the given path is not valid addr will be unchanged.
		 *
		 * \param path path to unix domain socket
		 * \param addr address structure to fill
		 * \exception SocketException thrown if path is not valid
		 */
		static void fillAddress( const std::string& path, sockaddr_un& addr);

		//! extracts a path string from the socket address structure
		static std::string extractPath( const sockaddr_un& addr, socklen_t len);
	};

} // namespace NET

#endif // NET_UnixSocket_h__
