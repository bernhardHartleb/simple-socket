#ifndef NET_SimpleSocket_h__
#define NET_SimpleSocket_h__

#include <sys/socket.h>
#include <string>
#include <exception>

namespace NET
{
	//! Signals a problem with the execution of a socket call
	class SocketException : public std::exception
	{
	public:
		/*!
		 * Construct a SocketException with a explanatory message.
		 * \param message explanatory message
		 * \param inclSysMsg true if system message from strerror(errno)
		 * should be postfixed to the user provided message
		 */
		SocketException( const std::string& message, bool inclSysMsg = true) throw();

		//! Provided just to guarantee that no exceptions are thrown.
		~SocketException() throw() {};

		//! Returns a C-string describing the cause of the current error.
		const char* what() const throw() { return m_message.c_str(); }

		//! Returns the glibc errno code of the current error.
		int errorCode() const throw() { return m_errorcode; }

	private:
		std::string m_message;
		int m_errorcode;
	};

	//! \cond internal
	//! data type used by transmit and receive functions
	typedef void raw_type;
	//! \endcond

	//! A socket representing a basic communication endpoint
	/*!
	 * All sockets must inherit from this class.
	 * It provides the basic interface for network communication.
	 */
	class SimpleSocket
	{
	public:
		enum ShutdownDirection
		{
			STOP_SEND = SHUT_WR,     ///< disable all variants of send() on the socket, receive calls still work
			STOP_RECEIVE = SHUT_RD,  ///< disable all variants of receive() on the socket, send calls still work
			STOP_BOTH = SHUT_RDWR    ///< disable all send() and receive() calls on the socket
		};

		~SimpleSocket();

		//! send data through a connected socket
		/*!
		 * send() can only be used on a socket that called connect() before.
		 * If you try to use send() on a not connected socket, SocketException
		 * will be thrown.
		 *
		 * If you are using a stream oriented socket (like TCPSocket),
		 * the operating system is allowed to send only a part of the packet
		 * you told it to send, so send() will return the number of bytes actually
		 * sent. It is your responsibility to resend the data not sent by send()
		 *
		 * If you are using a datagram oriented socket (like UDPSocket or SCTPSocket)
		 * the operating system will only send and receive complete datagrams, but send()
		 * will fail if you are trying to send too much data. In that case SocketException
		 * will be thrown.
		 *
		 * \param buffer data to be send
		 * \param len length of the data to be sent
		 * \return number of bytes sent
		 * \exception SocketException if sending went wrong
		 */
		int send( const void* buffer, size_t len);

		//! receive data from a bound socket
		/*!
		 * receive() can only be used on a socket that called bind() or
		 * connect() before.  If you try to use receive() on a not bound
		 * socket, SocketException will be thrown.
		 *
		 * If using a stream oriented Socket, receive can return a part of a
		 * received messge, e.g. if you send 100 bytes, it's possible you will receive
		 * 50 bytes two times in a row. However, the order of the sent data will be preserved.
		 *
		 * If you are using a datagram oriented sockets, you will only receive whole datagrams.
		 * But beware of using a too small buffer. If the receive buffer is too small for the
		 * received datagram, the data you didn't read in the receive call will be discared.
		 *
		 * If the remote host has closed the connection (on a connection based socket
		 * like TCP or SCTP) receive() will return 0. If you are using a connectionless
		 * protocol (like UDP) there is no way to determine wheter the connection has
		 * been closed by the remote host or not.
		 *
		 * \param buffer the buffer the received data will be written to
		 * \param len length of the provided buffer, receive will not read more than that
		 * \return number of bytes received
		 * \exception SocketException in case an error occured
		 */
		int receive( void* buffer, size_t len);

		//! receive data from a bound socket, return after the given timespan
		/*!
		 * timedReceive() can only be used on a socket that called bind() or
		 * connect before.  If you try to use receive() on a not bound socket,
		 * SocketException will be thrown.
		 *
		 * If using a stream oriented Socket, receive can return a part of a
		 * received messge, e.g. if you send 100 bytes, it's possible you will receive
		 * 50 bytes two times in a row. However, the order of the sent data will be preserved.
		 *
		 * If you are using a datagram oriented sockets, you will only receive whole datagrams.
		 * But beware of using a too small buffer. If the receive buffer is too small for the
		 * received datagram, the data you didn't read in the receive call will be discared.
		 *
		 * If the remote host has closed the connection (on a connection based socket
		 * like TCP or SCTP) receive() will return 0. If you are using a connectionless
		 * protocol (like UDP) there is no way to determine wheter the connection has
		 * been closed by the remote host or not.
		 *
		 * \param buffer the buffer the received data will be written to
		 * \param len length of the provided buffer, receive will not read more than that
		 * \param timeout the timeout in ms after which receive will give up and return
		 * \return number of bytes received, 0 on timeout
		 * \exception SocketException in case an error occured
		 */
		int timedReceive( void* buffer, size_t len, int timeout);

		//! Disconnect and unset any foreign addresses
		/*!
		 * This will ungracefully abort the connection by sending an RST frame.
		 * disconnect() should not be used under normal circumstances, as it will
		 * terminate the connection OOB, preventing the other end of the
		 * communication from retrieving data that would still be available in
		 * the OS buffer.
		 *
		 * For normal termination of a connection use shutdown().
		 *
		 * \exception SocketException thrown if unable to disconnect
		 */
		void disconnect();

		//! shutdown the connection in the specified direction
		/*!
		 * This will gracefully end a connection by sending a FIN frame.
		 * Use this to end a normal communication cycle. To abort a connection
		 * see disconnect().
		 *
		 * Depending on the specified ShutdownDirection, calls for that direction
		 * will stop working. Use this function if you want to have more control
		 * than just destroing the socket. It allows you to cut the connection
		 * in one direction, or both.
		 *
		 * If you use shutdown() on an unconnected socket, the corresponding
		 * calls will simply stop working.

		 * \param type the ShutdownDirection that be used
		 * \exception SocketException in case an error occured
		 */
		void shutdown( ShutdownDirection type);

		//! returns whether a peer disconnected
		/*!
		 * Will only work if you use a connection oriented, connected socket.
		 * Returns true if the peer disconnected. Use this function after
		 * a call to receive, returned 0 received bytes.
		 *
		 * \return true if a peer disconnected
		 */
		bool peerDisconnected() const;

	protected:
		enum SocketDomain
		{
			INTERNET = PF_INET,
			UNIX = PF_LOCAL,
			CAN = PF_CAN
		};

		enum SocketType
		{
			RAW = SOCK_RAW,
			STREAM = SOCK_STREAM,
			DATAGRAM = SOCK_DGRAM
		};

		//! enables return of an accepted socket
		SimpleSocket( int sockfd);

		//! allows a subclass to create new socket
		SimpleSocket( int domain, int type, int protocol);

		// socket descriptor
		int m_socket;
		bool m_peerDisconnected;

	private:
		// dont' allow
		SimpleSocket( const SimpleSocket&);
		const SimpleSocket& operator=( const SimpleSocket&);
	};

} // namespace NET

#endif // NET_SimpleSocket_h__
