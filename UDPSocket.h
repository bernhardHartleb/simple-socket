#ifndef NET_UDPSocket_h__
#define NET_UDPSocket_h__

#include "InternetSocket.h"

namespace NET
{
	//! UDP socket class
	class UDPSocket : public InternetSocket
	{
	public:
		// @{
		/*!
			Construct a UDP socket
			Eventually bind the socket to the specified port and address.
			Contructing the socket with port and/or address parameters will
			have the same effect as constructing it without parameters, and
			calling bind on that socket at a later time.

			\param localAddress local address
			\param localPort local port
			\exception SocketException thrown if unable to create UDP socket
		*/
		UDPSocket();
		UDPSocket( unsigned short localPort);
		UDPSocket( const std::string& localAddress, unsigned short localPort);
		// @}

		/*!
			Send the given buffer as a UDP datagram to the
			specified address / port.

			Can be substituted with send() if the socket is first connected to an
			address and port. If sendTo() is used on a connected socket, the
			destination parameters will be ignored, and the datagram will be sent
			to the connected socket.

			If the message you are trying to send is too long for one of the underlying protocols
			a SocketException will be thrown. Like send(), sendTo() blocks until the message was sent.

			\param buffer buffer to be written
			\param bufferLen number of bytes to write
			\param foreignAddress address (IP address or name) to send to
			\param foreignPort port number to send to
			\return true if send is successful
			\exception SocketException thrown if unable to send datagram
		*/
		void sendTo( const void* buffer, size_t bufferLen, const std::string& foreignAddress, unsigned short foreignPort);

		//@{
		/*!
			Read read up to bufferLen bytes data from this socket. The given buffer
			is where the data will be placed.

			In order to receive data, the socket has to be bound at least to a specific port.
			This can be done by passing a port number to the constructor,
			explicitly binding the socket, or sending data through the socket.

			\param buffer buffer to receive data
			\param bufferLen maximum number of bytes to receive
			\param sourceAddress address of datagram source
			\param sourcePort port of data source
			\param timeout timeout in milliseconds
			\return number of bytes received and -1 for error
			\exception SocketException thrown if unable to receive datagram
		*/
		int receiveFrom( void* buffer, size_t len, std::string& sourceAddress, unsigned short& sourcePort);
		int receiveFrom( void* buffer, size_t len, std::string& sourceAddress, unsigned short& sourcePort, int timeout);
		//@}

		/*!
			Set the multicast TTL
			\param multicastTTL multicast TTL
			\exception SocketException thrown if unable to set TTL
		*/
		void setMulticastTTL( unsigned char multicastTTL);

		/*!
			Join the specified multicast group

			The multicast group has to be a valid multicast IP Address (224.0.0.0/24)

			\param multicastGroup multicast group address to join
			\exception SocketException thrown if unable to join group
		*/
		void joinGroup( const std::string& multicastGroup);

		/*!
			Leave the specified multicast group

			The multicast group has to be a valid multicast IP Address (224.0.0.0/24)

			\param multicastGroup multicast group address to leave
			\exception SocketException thrown if unable to leave group
		*/
		void leaveGroup( const std::string& multicastGroup);
	};

} // namespace NET

#endif // NET_UDPSocket_h__
