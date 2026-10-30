#ifndef NET_InternetSocket_h__
#define NET_InternetSocket_h__

#include "SimpleSocket.h"

namespace NET
{
	//! Internet socket class
	/*!
	 * InternetSocket is the base class for all sockets that use address/port to
	 * identify communication endpoints. Those are all classes that are used for
	 * protocols that build on the IP protocol. (TCP/UDP/etc.).
	 * It provides the basic interface for connecting and binding the socket to
	 * an address/interface.
	 */
	class InternetSocket : public SimpleSocket
	{
	public:
		//! Enable or disable local address reuse
		/*!
		* SO_REUSEADDR allows binding a local address that may still be
		* associated with previous connections, e.g. TCP connections in
		* TIME_WAIT. This option should be set before bind().
		*
		* \param enable true to enable address reuse
		* \exception SocketException if changing the option fails
		*/
		void setReuseAddress( bool enable);

		//! establish a connection with the given foreign address and port
		/*!
		 * If you are using a connection oriented socket (like TCPSocket),
		 * you have to call connect() on that socket in order to send data.
		 * However, if you are using a non-connection oriented socket
		 * (like UDPSocket), calling connect on that socket will allow you to
		 * use the send() function like on a connection oriented socket.
		 * If not connected, you would have to use sendTo
		 *
		 * \param foreignAddress foreign address
		 * \param foreignPort foreign port
		 * \exception SocketException thrown if unable to establish connection
		 */
		void connect( std::string_view foreignAddress, unsigned short foreignPort);

		//! establish a connection with the given foreign address and port
		/*!
		 * Provides timeout for TCP socket connection establishment.
		 * When a timeout occurs, the OS may still continue the connection attempt
		 * in the background. Use disconnect() or destroy the socket to avoid this.
		 *
		 * \param foreignAddress foreign address
		 * \param foreignPort foreign port
		 * \param timeout the timeout in ms after which connect will give up and return
		 * \return 0 on timeout, 1 when connection is established
		 * \exception SocketException thrown if unable to establish connection
		 */
		int timedConnect( std::string_view foreignAddress, unsigned short foreignPort, int timeout);

		/*!
		 * \overload
		 * Instead of the richer function with more arguments, this bind()
		 * binds to any available interface / address
		 */
		void bind( unsigned short localPort = 0);

		//! bind socket to address / port
		/*!
		 * Set the local port to the specified port and the local address
		 * to an interface.
		 *
		 * bind() attaches the local endpoint of the socket to the specified
		 * interface and port.
		 * Typically all servers use bind() to attach to the local port specified by the
		 * used protocol. You just need to call bind if you want to restrict the connection
		 * to a specific local interface, or if you want to listen on a specific port.
		 * If you are just a client using connect() to connect to a server, calling
		 * bind is most likely unnecessary.
		 *
		 * To bind to a port < 1023 you need eighter superuser access, or the SUID bit set on
		 * your program.
		 *
		 * \param localAddress local address
		 * \param localPort local port
		 * \exception SocketException thrown if setting local port fails
		 */
		void bind( std::string_view localAddress, unsigned short localPort = 0);

		/*!
		 * Get the local address (after binding the socket).
		 * \return local address of socket
		 * \exception SocketException thrown if fetch fails
		 */
		std::string getLocalAddress() const;

		/*!
		 * Get the local port (after binding the socket).
		 * \return local port of socket
		 * \exception SocketException thrown if fetch fails
		 */
		unsigned short getLocalPort() const;

		/*!
		 * Get the foreign address. Call connect() before using this function.
		 * \return foreign address
		 * \exception SocketException thrown if unable to fetch foreign address
		 */
		std::string getForeignAddress() const;

		/*!
		 * Get the foreign port. Call connect() before using this function.
		 * \return foreign port
		 * \exception SocketException thrown if unable to fetch foreign port
		 */
		unsigned short getForeignPort() const;

		//! return wether this socket was created for IPv4 or IPv6
		AddressFamily addressFamily() const noexcept { return m_family; }

	protected:
		//! allows a subclass to create new socket
		InternetSocket( AddressFamily family, int type, int protocol);

		//! used to safely adopt internal socket descriptor
		struct AdoptSocketTag {};
		//! allows a subclass to take ownership of an accepted socket
		InternetSocket( AdoptSocketTag, int sockfd);

		InternetSocket( InternetSocket&& other) noexcept = default;
		InternetSocket& operator=( InternetSocket&& other) noexcept = default;

		// IPv4 or IPv6
		AddressFamily m_family;
	};

} // namespace NET

#endif // NET_InternetSocket_h__
