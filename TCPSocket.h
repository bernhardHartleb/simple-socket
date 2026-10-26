#ifndef NET_TCPSocket_h__
#define NET_TCPSocket_h__

#include "SocketHandle.h"
#include "InternetSocket.h"

namespace NET
{
	//! TCP socket class
	class TCPSocket : public InternetSocket
	{
	public:
		//! Handle for a new socket returned by accept()
		typedef SocketHandle<TCPSocket> Handle;

		/*!
		 * Construct a TCP socket
		 * \exception SocketException thrown if unable to create the socket
		 */
		TCPSocket( AddressFamily family = IPv4);

		/*!
		 * Construct a Socket from a Handle returned by accept()
		 * \exception SocketException thrown if handle is invalid
		 */
		explicit TCPSocket( Handle&& handle);

		//! Send the complete buffer
		using SimpleSocket::sendAll;

		//! Receive the complete requested buffer
		using SimpleSocket::receiveAll;

		//! Enable or disable TCP_NODELAY
		/*!
		* TCP_NODELAY disables Nagle's algorithm packet buffering. It
		* combines small writes into larger packets before transmission.
		* For small packets this can worsen latency and NODELAY is preferred.
		* This option can be used on connected sockets.
		*
		* \param enable true to enable nodelay
		* \exception SocketException if changing the option fails
		*/
		void setNoDelay( bool enable);

		//! listen for incoming connections
		/*!
		 * listen() can be called on a bound socket.
		 * The socket will then go into a passive state and accept incoming
		 * connections. To answer to an incoming connection call accept() or
		 * timedAccept() on the socket.
		 *
		 * The backlog is a buffer for incoming connections. If backlog is set
		 * to 0, the number of incoming connections is only limited by the
		 * operating system. If backlog is set to a value > 0, only the upper
		 * limit of connections is set, the OS can limit the number further.
		 *
		 * After starting to listen, use accept to accept incoming connections.
		 *
		 * \param backlog upper limit of pending incoming connections
		 * \exception SocketException
		 */
		void listen( int backlog = 5);

		//! wait for another socket to connect
		/*!
		 * accept() will block until a new connection arrives. It is used to
		 * create TCP server applications. On return it will return the handle
		 * to the incoming connection.
		 *
		 * accept can be only called on a listening socket.
		 *
		 * Use the returned handle to create a new TCPSocket to communicate
		 * with the new connected client. Regardless of what you do with a
		 * handle, accept() can be used to wait for next incoming connection
		 * after the handle was received.
		 *
		 * \return SocketHandle object to the new connection
		 * \exception SocketException
		 */
		Handle accept() const;

		//! wait for another socket to connect
		/*!
		 * timedAccept() will block until a new connection arrives, or the given
		 * timeout runs out. It is used to create TCP server applications. On
		 * return it will return the handle to the incoming connection.
		 *
		 * This handle might be invalid, so it should be checked before it is
		 * used. Creating a new socket object using an invalid handle will
		 * raise an exception.
		 *
		 * timedAccept() can only be called on a listening socket.
		 *
		 * Use the returned handle to create a new TCPSocket to communicate
		 * with the new connected client. Regardless of what you do with a
		 * handle, timedAccept() can be used to wait for next incoming
		 * connection after the handle was returned.
		 *
		 * \param timeout the timeout in ms after which accept will give up and return
		 * \return Handle object to the new connection
		 * \exception SocketException
		 */
		Handle timedAccept( int timeout) const;
	};

} // namespace NET

#endif // NET_TCPSocket_h__
