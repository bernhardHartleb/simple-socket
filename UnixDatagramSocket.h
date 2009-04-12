#ifndef NET_UnixDatagramSocket_h__
#define NET_UnixDatagramSocket_h__

#include "UnixSocket.h"

namespace NET
{
	//! Unix datagram socket class
	class UnixDatagramSocket : public UnixSocket
	{
	public:
		/*!
			Construct a Unix datagram socket
			\exception SocketException thrown if unable to create the socket
		*/
		UnixDatagramSocket();

		/*!
			Construct a UDP socket with the given local port and address
			\param localPath parth to bind
			\exception SocketException thrown if unable to create the socket
		*/
		UnixDatagramSocket( const std::string &localPath);

		/*!
			Send the given buffer as a UDP datagram to the
			specified address / port
			\param buffer buffer to be written
			\param bufferLen number of bytes to write
			\param foreignAddress address (IP address or name) to send to
			\param foreignPort port number to send to
			\return true if send is successful
			\exception SocketException thrown if unable to send datagram
		*/
		void sendTo( const void* buffer, int bufferLen, const std::string& foreignPath);

		/*!
			Read read up to bufferLen bytes data from this socket.  The given buffer
			is where the data will be placed
			\param buffer buffer to receive data
			\param bufferLen maximum number of bytes to receive
			\param sourceAddress address of datagram source
			\param sourcePort port of data source
			\return number of bytes received and -1 for error
			\exception SocketException thrown if unable to receive datagram
		*/
		int recvFrom( void* buffer, int bufferLen, std::string& sourcePath);
	};

} // namespace NET

#endif // NET_UnixDatagramSocket_h__
