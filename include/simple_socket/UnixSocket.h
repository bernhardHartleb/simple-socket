#ifndef NET_UnixSocket_h__
#define NET_UnixSocket_h__

#include "SimpleSocket.h"

namespace NET
{
	//! Unix domain socket class
	/*!
	 *	Base class for sockets identified by files in the filessystem.
	 */
	class UnixSocket : public SimpleSocket
	{
	public:
		~UnixSocket();

		/*!
		 * Establish a socket connection with the given socket
		 * \param foreignPath the path of the remote socket
		 * \exception SocketException thrown if unable to establish connection
		 */
		void connect( std::string_view foreignPath);

		/*!
		 * Set the local path to the specified path
		 * \param localPath specifies where the socket should be bound
		 * \exception SocketException thrown if setting local path fails
		 */
		void bind( std::string_view localPath);

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

		//! used to safely adopt internal socket descriptor
		struct AdoptSocketTag {};
		//! allows a subclass to take ownership of an accepted socket
		UnixSocket( AdoptSocketTag, int sockfd);

		UnixSocket( UnixSocket&& other) noexcept;
		UnixSocket& operator=( UnixSocket&& other) noexcept;

		// track created socket file
		std::string m_boundPath;
	};

} // namespace NET

#endif // NET_UnixSocket_h__
