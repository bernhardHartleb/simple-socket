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
		 * Construct a Unix datagram socket
		 * \exception SocketException thrown if unable to create the socket
		 */
		UnixDatagramSocket();

		/*!
		 * Send the given buffer as a datagram to the specified address / port.
		 *
		 * \param buffer data to be send
		 * \param len number of bytes to write
		 * \param foreignPath filename of the datagram socket the data should be sent to
		 * \return true if send is successful
		 * \exception SocketException thrown if unable to send datagram
		 */
		void sendTo( const void* buffer, size_t len, const std::string& foreignPath);

		/*!
		 * Read read up to bufferLen bytes data from this socket. The given
		 * buffer is where the data will be placed.
		 *
		 * \param buffer buffer to receive data
		 * \param len maximum number of bytes to receive
		 * \param sourcePath path where the data originated
		 * \return number of bytes received
		 * \exception SocketException thrown if unable to receive datagram
		 */
		int receiveFrom( void* buffer, size_t len, std::string& sourcePath);

		/*!
		 * Read read up to bufferLen bytes data from this socket. The given
		 * buffer is where the data will be placed. If no host has sent a
		 * datagram before the timeout runs out, the function will return
		 * without changing the buffer.
		 *
		 * \param buffer buffer to receive data
		 * \param len maximum number of bytes to receive
		 * \param sourcePath path where the data originated
		 * \param timeout timeout in milliseconds
		 * \return number of bytes received, 0 on timeout
		 * \exception SocketException thrown if unable to receive datagram
		 */
		int timedReceiveFrom( void* buffer, size_t len, std::string& sourcePath, int timeout);
	};

} // namespace NET

#endif // NET_UnixDatagramSocket_h__
