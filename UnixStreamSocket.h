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
		typedef SocketHandle<TCPSocket> Handle;

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

		/*!
		 * Send the complete buffer unless the associated peer becomes
		 * unavailable.
		 *
		 * \return number of bytes sent
		 * \exception SocketException on errors other than peer loss
		 */
		int sendAll( const void* buffer, size_t len);

		/*!
		 * Put a bound socket into listening state.
		 *
		 * \param backlog upper limit of pending incoming connections
		 * \exception SocketException if listen fails
		 */
		void listen( int backlog = 5);

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
	};

} // namespace NET

#endif // NET_UnixStreamSocket_h__
