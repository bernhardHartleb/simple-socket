#ifndef NET_UDPSocket_h__
#define NET_UDPSocket_h__

#include "InternetSocket.h"

namespace NET
{
	//! UDP socket class
	class UDPSocket : public InternetSocket
	{
	public:
		/*!
		 * Construct a UDP socket and enable broadcast capabilities
		 * \exception SocketException thrown if unable to create the socket
		 */
		UDPSocket();

		/*!
		 * Send the given buffer as a UDP datagram to the
		 * specified address / port.
		 *
		 * Can be substituted with send() if the socket is first connected to
		 * an address and port. If sendTo() is used on a connected socket, the
		 * destination parameters will be ignored, and the datagram will be sent
		 * to the connected socket.
		 *
		 * If the message you are trying to send is too long for one of
		 * the underlying protocols a SocketException will be thrown.
		 * Like send(), sendTo() blocks until the message was sent.
		 *
		 * \param buffer data to be send
		 * \param len number of bytes to write
		 * \param foreignAddress address (IP address or name) to send to
		 * \param foreignPort port number to send to
		 * \exception SocketException thrown if unable to send datagram
		 */
		void sendTo( const void* buffer, size_t len, const std::string& foreignAddress, unsigned short foreignPort);

		/*!
		 * Read read up to len bytes data from this socket. The given buffer
		 * is where the data will be placed.
		 *
		 * In order to receive data, the socket has to be bound at least to a specific port.
		 * This is done by explicitly binding the socket, or sending data through the socket.
		 *
		 * \param buffer buffer to receive data
		 * \param len maximum number of bytes to receive
		 * \param sourceAddress address of datagram source
		 * \param sourcePort port of data source
		 * \return number of bytes received
		 * \exception SocketException thrown if unable to receive datagram
		 */
		int receiveFrom( void* buffer, size_t len, std::string& sourceAddress, unsigned short& sourcePort);

		/*!
		 * Read read up to len bytes data from this socket. The given
		 * buffer is where the data will be placed. If no host has sent a
		 * datagram before the timeout runs out, the function will return
		 * without changing the buffer.
		 *
		 * In order to receive data, the socket has to be bound at least to a specific port.
		 * This is done by explicitly binding the socket, or sending data through the socket.
		 *
		 * \param buffer buffer to receive data
		 * \param len maximum number of bytes to receive
		 * \param sourceAddress address of datagram source
		 * \param sourcePort port of data source
		 * \param timeout timeout in milliseconds
		 * \return number of bytes received, 0 on timeout
		 * \exception SocketException thrown if unable to receive datagram
		 */
		int timedReceiveFrom( void* buffer, size_t len, std::string& sourceAddress, unsigned short& sourcePort, int timeout);

		/*!
		 * Set the multicast TTL
		 * \param multicastTTL multicast TTL
		 * \exception SocketException thrown if unable to set TTL
		 */
		void setMulticastTTL( unsigned char multicastTTL);

		/*!
		 * Join the specified multicast group
		 *
		 * The multicast group has to be a valid multicast IP Address (224.0.0.0/24)
		 *
		 * \param multicastGroup multicast group address to join
		 * \exception SocketException thrown if unable to join group
		 */
		void joinGroup( const std::string& multicastGroup);

		/*!
		 * Leave the specified multicast group
		 *
		 * The multicast group has to be a valid multicast IP Address (224.0.0.0/24)
		 *
		 * \param multicastGroup multicast group address to leave
		 * \exception SocketException thrown if unable to leave group
		 */
		void leaveGroup( const std::string& multicastGroup);
	};

} // namespace NET

#endif // NET_UDPSocket_h__
