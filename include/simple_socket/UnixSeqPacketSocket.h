#ifndef NET_UnixSeqPacketSocket_h__
#define NET_UnixSeqPacketSocket_h__

#include "UnixSocket.h"
#include <optional>

namespace NET
{
	//! Unix domain sequential packet (SOCK_SEQPACKET) socket class
	class UnixSeqPacketSocket : public UnixSocket
	{
	public:
		/*!
		 * Construct a Unix domain seqpacket socket.
		 * \exception SocketException thrown if unable to create the socket
		 */
		UnixSeqPacketSocket();

		//! Take ownership of an existing native Unix seqpacket socket descriptor
		/*!
		 * This is intended for sockets created outside simple-socket, for example
		 * listening sockets supplied through systemd socket activation.
		 *
		 * The descriptor must refer to an AF_UNIX SOCK_SEQPACKET socket. On success
		 * ownership is transferred to the returned object and the descriptor will
		 * be closed when that object is destroyed.
		 *
		 * An adopted socket does not claim ownership of its filesystem path.
		 *
		 * \param sockfd native socket descriptor to adopt
		 * \return UnixSeqPacketSocket owning sockfd
		 * \exception SocketException if sockfd is invalid or not a Unix seqpacket socket
		 */
		[[nodiscard]] static UnixSeqPacketSocket adoptNativeHandle( int sockfd);

		// Listen for incoming connections
		using SimpleSocket::listen;

		// Complete async connect attempt
		using SimpleSocket::finishConnect;

		/*!
		 * Accept an incoming connection.
		 * In blocking mode it will wait until the new connection arrives.
		 * In non-blocking mode the optional return value may be null right away.
		 *
		 * accept() can be only called on a listening socket.
		 *
		 * \return optional UnixSeqPacketSocket object owning the accepted socket
		 * \exception SocketException
		 */
		[[nodiscard]] std::optional<UnixSeqPacketSocket> accept() const;

		/*!
		 * Wait up to timeout milliseconds for an incoming connection.
		 *
		 * timedAccept() can only be called on a listening socket.
		 *
		 * \param timeout the timeout in ms after which accept will give up
		 * \return optional UnixSeqPacketSocket object to the new connection, nullopt on timeout
		 * \exception SocketException on other errors
		 */
		[[nodiscard]] std::optional<UnixSeqPacketSocket> timedAccept( int timeout) const;

	protected:
		// allow construction by accept()
		using UnixSocket::UnixSocket;
	};

} // namespace NET

#endif // NET_UnixSeqPacketSocket_h__
