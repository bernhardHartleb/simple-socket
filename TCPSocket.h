#ifndef NET_TCPSocket_h__
#define NET_TCPSocket_h__

#include "InternetSocket.h"

namespace NET
{
	//! TCP socket class
	class TCPSocket : public InternetSocket
	{
	public:
		//! handle for new socket returned on accept call
		class Handle
		{
		public:
			friend class TCPSocket;
			operator bool () { return m_sockfd != 0; }
		private:
			Handle( int sock) : m_sockfd(sock) {}
			int m_sockfd;
		};

		/*!
			Construct a TCP socket
			\exception SocketException thrown if unable to create the socket
		*/
		TCPSocket();

		/*!
			Construct a TCP socket and connect to the given foreign port and address
			\param foreignAddress local address
			\param foreignPort local port
			\exception SocketException thrown if unable to create UDP socket
		*/
		TCPSocket( const std::string &foreignAddress, unsigned short foreignPort);

		//! Construct a Socket from a TCPSocketHandle returned by accept()
		TCPSocket( Handle handle);

		//! send data through a connected socket
		/*!
		 * sendAll() can only be used on a socket that called connect() before (Or was contructed connected)
		 * If you try to use sendAll() on a not connected socket, SocketException will be thrown.
		 *
		 * When using a stream oriented socket, the operating system is allowed to send only
		 * a bit of the data you requested it to send. Because of this, you would have to
		 * check the number of sent bytes when using send(), and resend unsent data manually.
		 * sendAll() takes that responsibility from you, and resends as long as it needs, to
		 * completely send the data given to it.
		 *
		 * \param buffer data to send
		 * \param len length of the data to be sent
		 * \return returns the number of sent bytes
		 * \exception SocketException
		 */
		int sendAll( const void* buffer, size_t len);

		//! listen for incoming connections
		/*!
		 * listen can be called on a bound socket. 
		 * The socket will then go into a passive state and accept incoming
		 * connections. To answer to an incoming connection call accept /
		 * timedAccept on the socket.
		 *
		 * The backlog is a buffer for incoming connections. If backlog is set
		 * to 0, the number of incoming connections is only limited by the
		 * operating system. If backlog is set to a value > 0, only the upper
		 * limit of connections is set, the OS can limit the number further.
		 *
		 * after starting to listen, use accept to accept incoming connections.
		 *
		 * \param backlog upper limit of waiting incoming connections
		 * \exception SocketException
		 */
		void listen( int backlog = 0);

		//@{
		//! wait for another socket to connect
		/*!
		 * accept will block until a new connection arrives. It is used to
		 * create TCP server applications. On return it will return the handle
		 * to the incoming connection.
		 *
		 * accept can be called on a listening socket.
		 *
		 * Use this handle to create a new TCPSocket to communicate with the
		 * new connected client. accept can be used to wait for next incoming
		 * connection after the handle was received.
		 *
		 * \param timeout the timeout after which accept will give up and return
		 * \return Handle object to the new connection
		 */
		Handle accept() const;
		Handle timedAccept( int timeout) const;
		//@}
	};

} // namespace NET

#endif // NET_TCPSocket_h__
