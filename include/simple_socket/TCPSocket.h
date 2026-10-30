#ifndef NET_TCPSocket_h__
#define NET_TCPSocket_h__

#include "InternetSocket.h"
#include <optional>

namespace NET
{
	//! TCP socket class
	class TCPSocket : public InternetSocket
	{
	public:
		/*!
		 * Construct a TCP socket
		 * \exception SocketException thrown if unable to create the socket
		 */
		TCPSocket( AddressFamily family = IPv4);

		//! Listen for incoming connections
		using SimpleSocket::listen;

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

		//! wait for another socket to connect
		/*!
		 * accept() will block until a new connection arrives. It is used to
		 * create TCP server applications. On return it will return the handle
		 * to the incoming connection.
		 *
		 * accept can be only called on a listening socket.
		 *
		 * \return TCPSocket object to the new connection
		 * \exception SocketException
		 */
		TCPSocket accept() const;

		//! wait for another socket to connect
		/*!
		 * timedAccept() will block until a new connection arrives, or the given
		 * timeout runs out. It is used to create TCP server applications. On
		 * return it will return the handle to the incoming connection.
		 *
		 * timedAccept() can only be called on a listening socket.
		 *
		 * \param timeout the timeout in ms after which accept will give up and return
		 * \return optional TCPSocket object to the new connection, nullopt on timeout
		 * \exception SocketException
		 */
		std::optional<TCPSocket> timedAccept( int timeout) const;

		//! Send the complete buffer
		using SimpleSocket::sendAll;

		//! Receive the complete requested buffer
		using SimpleSocket::receiveAll;

	protected:
		// allow construction by accept()
		using InternetSocket::InternetSocket;
	};

} // namespace NET

#endif // NET_TCPSocket_h__
