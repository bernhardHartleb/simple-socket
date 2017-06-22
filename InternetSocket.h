#ifndef NET_InternetSocket_h__
#define NET_InternetSocket_h__

#include "SimpleSocket.h"

struct sockaddr_in;

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
		//! establish a connection with the given foreign address and port
		/*!
		 * If you are using a connection oriented socket (like TCPSocket),
		 * you have to call connect() on that socket in order to send data.
		 * However, if you are using a non-connection oriented socket
		 * (like UDPSocket), calling connect on that socket will allow you to
		 * use the send() function like on a connection oriented socket.
		 * If not connected, you would have to use sendTo
		 *
		 * \param foreignAddress foreign address (IP address or name)
		 * \param foreignPort foreign port
		 * \exception SocketException thrown if unable to establish connection
		 */
		void connect( const std::string& foreignAddress, unsigned short foreignPort);

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
		void bind( const std::string& localAddress, unsigned short localPort = 0);

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

	protected:
		//! create socket from a SocketHandle returned by an accept() call
		explicit InternetSocket( int sockfd);

		//! allows a subclass to create new socket
		InternetSocket( int type, int protocol);

		/*!
		 * Fill an address structure with the given address and port number.
		 * If the given address is not a valid IPv4 address, it will be resolved
		 * by hostname or DNS lookup. addr will be unchanged if this resolve fails.
		 *
		 * \param address IPv4 domain name or address
		 * \param port IP port number to fill in
		 * \param addr address structure to fill
		 * \exception SocketException thrown if unable to resolve a hostname
		 */
		static void fillAddress( const std::string& address, unsigned short port, sockaddr_in& addr);
	};

} // namespace NET

#endif // NET_InternetSocket_h__
