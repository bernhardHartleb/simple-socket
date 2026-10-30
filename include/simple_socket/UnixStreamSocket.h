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

		//! Listen for incoming connections
		using SimpleSocket::listen;

		/*!
		 * Wait for and accept an incoming connection.
		 *
		 * \return  UnixStreamSocket object owning the accepted socket
		 * \exception SocketException if accept fails
		 */
		UnixStreamSocket accept() const;

		/*!
		 * Wait up to timeout milliseconds for an incoming connection.
		 *
		 * \return optional UnixStreamSocket object to the new connection, nullopt on timeout
		 * \exception SocketException on errors
		 */
		std::optional<UnixStreamSocket> timedAccept( int timeout) const;

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
