#ifndef NET_UnixStreamSocket_h__
#define NET_UnixStreamSocket_h__

#include "UnixSocket.h"
#include <optional>

namespace NET
{
	//! Unix domain stream socket class
	class UnixStreamSocket : public UnixSocket
	{
	public:
		/*!
		 * Construct a Unix domain stream socket.
		 * \exception SocketException thrown if unable to create the socket
		 */
		UnixStreamSocket();

		//! Take ownership of an existing native Unix stream socket descriptor
		/*!
		 * This is intended for sockets created outside simple-socket, for example
		 * listening sockets supplied through systemd socket activation.
		 *
		 * The descriptor must refer to an AF_UNIX SOCK_STREAM socket. On success
		 * ownership is transferred to the returned object and the descriptor will
		 * be closed when that object is destroyed.
		 *
		 * An adopted socket does not claim ownership of its filesystem path.
		 *
		 * \param sockfd native socket descriptor to adopt
		 * \return UnixStreamSocket owning sockfd
		 * \exception SocketException if sockfd is invalid or not a Unix stream socket
		 */
		[[nodiscard]] static UnixStreamSocket adoptNativeHandle( int sockfd);

		//! Listen for incoming connections
		using SimpleSocket::listen;

		/*!
		 * Wait for and accept an incoming connection.
		 *
		 * \return  UnixStreamSocket object owning the accepted socket
		 * \exception SocketException if accept fails
		 */
		[[nodiscard]] std::optional<UnixStreamSocket> accept() const;

		/*!
		 * Wait up to timeout milliseconds for an incoming connection.
		 *
		 * \return optional UnixStreamSocket object to the new connection, nullopt on timeout
		 * \exception SocketException on errors
		 */
		[[nodiscard]] std::optional<UnixStreamSocket> timedAccept( int timeout) const;

		//! Send the complete buffer
		using SimpleSocket::sendAll;

		//! Receive the complete requested buffer
		using SimpleSocket::receiveAll;

	protected:
		// allow construction by accept()
		using UnixSocket::UnixSocket;
	};

} // namespace NET

#endif // NET_UnixStreamSocket_h__
