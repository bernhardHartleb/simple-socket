#ifndef NET_UnixStreamSocket_h__
#define NET_UnixStreamSocket_h__

#include "SocketHandle.h"
#include "UnixSocket.h"

namespace NET
{
	//! Unix domain stream socket class
	class UnixStreamSocket : public UnixSocket
	{
	public:
		//! Handle for a new socket returned by accept()
		typedef SocketHandle<UnixStreamSocket> Handle;

		/*!
		 * Construct a Unix domain stream socket.
		 * \exception SocketException thrown if unable to create the socket
		 */
		UnixStreamSocket();

		/*!
		 * Construct a socket from a Handle returned by accept().
		 * Ownership of the descriptor is transferred from the handle.
		 * The accepted socket does not own the listening socket's pathname.
		 *
		 * \exception SocketException thrown if handle is invalid
		 */
		explicit UnixStreamSocket( Handle&& handle);

		//! Listen for incoming connections
		using SimpleSocket::listen;

		/*!
		 * Wait for and accept an incoming connection.
		 *
		 * \return handle owning the accepted socket
		 * \exception SocketException if accept fails
		 */
		Handle accept() const;

		/*!
		 * Wait up to timeout milliseconds for an incoming connection.
		 *
		 * \return valid handle on success, invalid handle on timeout
		 * \exception SocketException on errors
		 */
		Handle timedAccept( int timeout) const;

		//! Send the complete buffer
		using SimpleSocket::sendAll;

		//! Receive the complete requested buffer
		using SimpleSocket::receiveAll;
	};

} // namespace NET

#endif // NET_UnixStreamSocket_h__
