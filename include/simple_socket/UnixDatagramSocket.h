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

		//! Take ownership of an existing native Unix datagram socket descriptor
		/*!
		 * This is intended for sockets created outside simple-socket, for example
		 * listening sockets supplied through systemd socket activation.
		 *
		 * The descriptor must refer to an an AF_UNIX SOCK_DGRAM socket. On success
		 * ownership is transferred to the returned object and the descriptor will
		 * be closed when that object is destroyed.
		 *
		 * An adopted socket does not claim ownership of its filesystem path.
		 *
		 * \param sockfd native socket descriptor to adopt
		 * \return UnixDatagramSocket owning sockfd
		 * \exception SocketException if sockfd is invalid or not a Unix dgram socket
		 */
		[[nodiscard]] static UnixDatagramSocket adoptNativeHandle( int sockfd);

		/*!
		 * Send the given buffer as a datagram to the specified address / port.
		 *
		 * \param buffer data to be send
		 * \param len number of bytes to write
		 * \param foreignPath filename of the datagram socket the data should be sent to
		 * \return IOResult
		 * \exception SocketException thrown if unable to send datagram
		 */
		IOResult sendTo( const void* buffer, size_t len, std::string_view foreignPath);

		/*!
		 * Read read up to len bytes data from this socket. The given
		 * buffer is where the data will be placed.
		 *
		 * \param buffer buffer to receive data
		 * \param len maximum number of bytes to receive
		 * \param sourcePath path where the data originated
		 * \return IOResult
		 * \exception SocketException thrown if unable to receive datagram
		 */
		IOResult receiveFrom( void* buffer, size_t len, std::string& sourcePath);

		/*!
		 * Read read up to len bytes data from this socket. The given
		 * buffer is where the data will be placed. If no host has sent a
		 * datagram before the timeout runs out, the function will return
		 * without changing the buffer.
		 *
		 * \param buffer buffer to receive data
		 * \param len maximum number of bytes to receive
		 * \param sourcePath path where the data originated
		 * \param timeout timeout in milliseconds
		 * \return IOResult
		 * \exception SocketException thrown if unable to receive datagram
		 */
		[[nodiscard]] IOResult timedReceiveFrom( void* buffer, size_t len, std::string& sourcePath, int timeout);

	protected:
		// allow construction by adopt
		using UnixSocket::UnixSocket;
	};

} // namespace NET

#endif // NET_UnixDatagramSocket_h__
