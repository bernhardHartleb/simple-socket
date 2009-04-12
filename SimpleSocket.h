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
			Construct a SocketException with a explanatory message.
			\param message explanatory message
			\param inclSysMsg true if system message from strerror(errno)
			should be postfixed to the user provided message
		*/
		SocketException( const std::string& message, bool inclSysMsg = true) throw();

		//! Provided just to guarantee that no exceptions are thrown.
		~SocketException() throw() {};

		/*!
			Get the exception message
			\return exception message
		*/
		const char* what() const throw() { return m_message.c_str(); }

		/*!
		 * Get the glib error code
		 * \return error code
		 */
		int getErrorcode() const throw() { return errorcode; }
	private:
		std::string m_message;	// Exception message
		int errorcode;
	};

	typedef void raw_type; // data type used by transmit and receive functions

	//! A socket representing a basic communication endpoint
	/*!
		All sockets must inherit from this class.
		It provides the basic interface for network communication.
	*/
	class SimpleSocket
	{
	public:
		enum SocketDomain
		{
			INTERNET = PF_INET,
			UNIX = PF_LOCAL
		};

		enum SocketType
		{
			STREAM = SOCK_STREAM,
			DATAGRAM = SOCK_DGRAM
		};

		enum ShutdownDirection
		{
			STOP_SEND = SHUT_WR,		///< disable all variants of send() on the socket, receive calls still work
			STOP_RECEIVE = SHUT_RD,		///< disable all variants of receive() on the socket, send calls still work
			STOP_BOTH = SHUT_RDWR		///< disable all send() and receive() calls on the socket
		};

		SimpleSocket( int domain, int type, int protocol);
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
		 * \param buffer data to send
		 * \param len length of the data to be sent
		 * \return returns the number of sent bytes
		 * \exception SocketException if sending goes wrong
		 */
		int send( const void* buffer, size_t len);

		//@{
		//! will receive data from a bound socket
		/*!
		 * \name Receive Functions
		 *
		 * receive() can only be used on a socket that called bind() before.
		 * If you try to use receive() on a not bound socket, SocketException
		 * will be thrown.
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
		 * \param timeout the timeout after which receive will give up and return
		 * \return int number of received bytes
		 * \exception SocketException
		 */
		int receive( void* buffer, size_t len);
		int timedReceive( void* buffer, size_t len, int timeout);
		//@}

		//! will shutdown the connection in the specified direction
		/*!
		 * Depending on the specified ShutdownDirection, calls for that direction
		 * will stop working.
		 * Use this function if you want to have a little more control than just destroing
		 * the socket. It allows you to cut the connection in one direction, or both.
		 *
		 * If shutdown fails SocketException will be thrown.
		 *
		 * If you use shutdown() on an unconnected socket, the corresponding calls will simply
		 * stop working. But shutdown can be used on a connected UDP socket.
		 */
		void shutdown( ShutdownDirection type);

		//! tells wheter the peer disconnected or not
		/*!
		 * Will only work if you use an connection oriented, connected socket.
		 * Returns true if the peer disconnected. Use this function after
		 * receive, returned 0.
		 *
		 * \return bool true if the peer disconnected
		 */
		bool peerDisconnected();
	protected:
		//! enables return of an accepted socket
		SimpleSocket( int sockfd);

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
