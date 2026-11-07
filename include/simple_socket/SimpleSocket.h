#ifndef NET_SimpleSocket_h__
#define NET_SimpleSocket_h__

#include <string>
#include <string_view>
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
		 * \param inclErrno if true system message from strerror(errno) is postfixed
		 */
		SocketException( std::string_view message, bool inclErrno = true);

		/*!
		 * Construct a SocketException with a explanatory message.
		 * \param message explanatory message
		 * \param error system message from strerror(error) is postfixed
		 */
		SocketException( std::string_view message, int error);

		SocketException( const SocketException&) = default;

		//! Provided just to guarantee that no exceptions are thrown.
		~SocketException() noexcept {};

		//! Returns a C-string describing the cause of the current error.
		const char* what() const noexcept { return m_message.c_str(); }

		//! Returns the glibc errno code of the current error.
		int errorCode() const noexcept { return m_errorcode; }

	private:
		std::string m_message;
		int m_errorcode;
	};

	//! used by internet socket types
	enum AddressFamily
	{
		IPv4,
		IPv6
	};

	// forward declare internal type
	class SockAddress;

	//! status of the I/O operation
	enum class [[nodiscard]] IOStatus
	{
		Success,
		WouldBlock,
		Timeout,
		PeerDisconnected,
		DestinationUnavailable
	};

	/*!
	 * Success
	 *  bytes is meaningful, including zero
	 *
	 * WouldBlock
	 *  bytes == 0
	 *  only produced when socket is non-blocking
	 *
	 * Timeout
	 *  bytes == 0
	 *
	 * PeerDisconnected
	 *  bytes == 0
	 *  only produced for connection-oriented sockets
	 *
	 * DestinationUnavailable
	 *  bytes == 0
	 *  primarily datagram semantics
	 */
	struct [[nodiscard]] IOResult
	{
		IOStatus status;
		size_t bytes;

		//! simple check for successfull IO
		explicit operator bool() const noexcept { return status == IOStatus::Success; }
	};

	//! result of the connection attempt
	enum class [[nodiscard]] ConnectResult
	{
		Connected,
		InProgress,
		Timeout,
		Refused,
		Aborted,
		Unreachable
	};

	//! direction to close for a stream socket
	enum ShutdownDirection
	{
		STOP_RECEIVE,    ///< disable all variants of receive() on the socket, send calls still work
		STOP_SEND,       ///< disable all variants of send() on the socket, receive calls still work
		STOP_BOTH        ///< disable all send() and receive() calls on the socket
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
		SimpleSocket( const SimpleSocket&) = delete;
		SimpleSocket& operator=( const SimpleSocket&) = delete;

		//! Return the native descriptor. Ownership remains with the object.
		int nativeHandle() noexcept { return m_socket; }

		//! Duplicates the underlaying native socket descriptor
		[[nodiscard]] int duplicateNativeHandle() const;

		//! Returns true if socket is in a usable (non-moved) state and open
		bool valid() const noexcept;

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
		 * \return IOResult
		 * \exception SocketException if sending went wrong
		 */
		IOResult send( const void* buffer, size_t len);

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
		 * like TCP) receive() will return PeerDisconnected in the result status.
		 * If you are using a connectionless protocol (like UDP) you may get
		 * DestinationUnavailable in the result status when the peer refuses via ICMP.
		 *
		 * \param buffer the buffer the received data will be written to
		 * \param len length of the provided buffer, receive will not read more than that
		 * \return IOResult
		 * \exception SocketException in case an error occured
		 */
		IOResult receive( void* buffer, size_t len);

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
		 * like TCP) receive() will return PeerDisconnected in the result status.
		 * If you are using a connectionless protocol (like UDP) you may get
		 * DestinationUnavailable in the result status when the peer refuses via ICMP.
		 *
		 * \param buffer the buffer the received data will be written to
		 * \param len length of the provided buffer, receive will not read more than that
		 * \param timeout the timeout in ms after which receive will give up and return
		 * \return IOResult
		 * \exception SocketException in case an error occured
		 */
		IOResult timedReceive( void* buffer, size_t len, int timeout);

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
		 *
		 * \param type the ShutdownDirection that be used
		 * \exception SocketException in case an error occured
		 */
		void shutdown( ShutdownDirection type);

		//! returns whether a currently associated peer disconnected
		/*!
		 * Returns whether the OS currently reports hangup or error for the stream peer.
		 * A false result does not guarantee that the remote peer is reachable.
		 *
		 * \return true if a peer disconnected or is no longer connected
		 */
		[[nodiscard]] bool peerDisconnected() const;

		//! returns true if socket is in blocking mode
		bool isBlocking() const;

		//! set socket to blocking or non-blocking I/O operations
		/*!
		 * A non-blocking socket will return from all I/O operations right away.
		 * The IOStatus WouldBlock is used to signify no work can be done
		 * at the moment. An event handler can use poll(), epoll(), etc. to
		 * monitor the socket for readyness for the next operation.
		 *
		 * All timed operations provided by socket classes are not affected.
		 * Timed calls wait up to timeout ms for even with non-blocking sockets.
		 */
		void setBlocking(bool blocking);

	protected:
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
		 * \param backlog upper limit of pending incoming connections
		 * \exception SocketException
		 */
		void listen( int backlog = 5);

		//! send data through a connected socket
		/*!
		 * sendAll() can only be used on a socket that called connect()
		 * before. If you try to use sendAll() on a unconnected socket,
		 * a SocketException will be thrown.
		 *
		 * When using a stream oriented socket, the operating system is allowed
		 * to send only a part of the data you requested it to send.
		 * Because of this, you would have to check the number of sent bytes
		 * when using send(), and resend unsent data manually. sendAll() takes
		 * that responsibility from you and resends as long as it needs, to
		 * completely send the data given to it.
		 *
		 * \param buffer data to be send
		 * \param len length of the data to be sent
		 * \return IOResult
		 * \exception SocketException
		 */
		IOResult sendAll( const void* buffer, size_t len);

		//! receive exactly the requested amount unless the peer disconnects
		/*!
		 * receiveAll() repeatedly calls receive() until len bytes have been
		 * received. This is useful for fixed-size data on TCP, where one receive()
		 * call is not guaranteed to return all bytes sent by the peer.
		 *
		 * If the peer closes or resets the connection before len bytes arrive, the
		 * function returns an error status with the number of bytes received so far.
		 *
		 * \param buffer buffer the received data will be written to
		 * \param len number of bytes to receive
		 * \return IOResult
		 * \exception SocketException on receive errors other than peer loss
		 */
		IOResult receiveAll( void* buffer, size_t len);

		//! allows a subclass to create a new socket
		SimpleSocket( int domain, int type, int protocol);

		//! used to safely adopt internal socket descriptor
		struct AdoptSocketTag {};
		//! allows a subclass to take ownership of an existing socket
		SimpleSocket( AdoptSocketTag, int sockfd, int type);

		//! allows moving concrete classes
		SimpleSocket( SimpleSocket&& other) noexcept;
		SimpleSocket& operator=( SimpleSocket&& other) noexcept;

		//! protected destructor to avoid delete via base class
		~SimpleSocket();

		//! \cond internal

		//! sendTo for datagram sockets
		IOResult sendTo( const void* buffer, size_t len, const SockAddress& addr);

		//! receiveFrom for datagram sockets
		IOResult receiveFrom( void* buffer, size_t len, SockAddress& addr);

		//! direct accept syscall
		int acceptNative() const;

		//! retrying accept for use by stream sockets
		int accept() const;

		//! retrying accept with timeout for use by stream sockets
		int timedAccept( int timeout) const;

		//! address family agnostic connect
		ConnectResult connect( const SockAddress& addr);

		//! address family agnostic timed connect
		ConnectResult timedConnect( const SockAddress& addr, int timeout);

		//! \endcond

		//! check if external socket descriptor to fits the given parameters
		static void validateSocket( int sockfd, int type, int protocol);

		// socket descriptor
		int m_socket;
		bool m_connectionOriented;
	};

} // namespace NET

#endif // NET_SimpleSocket_h__
