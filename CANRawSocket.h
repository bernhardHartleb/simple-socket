#ifndef NET_CANRawSocket_h__
#define NET_CANRawSocket_h__

#include "CANSocket.h"

namespace NET
{
	//! Raw CAN socket class
	class CANRawSocket : public CANSocket
	{
	public:
		/*!
		 * Construct a raw CAN socket
		 * \exception SocketException thrown if unable to create the socket
		 */
		CANRawSocket();

		//! (overload) send can_frame through a connected socket
		void send( const ::can_frame& frame);

		//! (overload) receive can frame from a bound socket
		int receive( ::can_frame& frame);

		//! (overload) receive can frame from a bound socket with timeout
		int timedReceive( ::can_frame& frame, int timeout);

		/*!
		 * Send the given CAN frame using the specified interface.
		 *
		 * Can be substituted with send() if the socket is first connected to
		 * any interface. If sendTo() is used on a connected socket, the
		 * destination parameters will be ignored, and the datagram will be sent
		 * to the connected socket.
		 *
		 * Like send(), sendTo() blocks until the message was sent.
		 *
		 * \param frame CAN frame structure to be send
		 * \param interface specifies the CAN interface to send used for sending
		 * \exception SocketException thrown if unable to send frame
		 */
		void sendTo( const ::can_frame& frame, std::string_view interface);

		/*!
		 * Read one CAN frame from this socket.
		 *
		 * In order to receive data, the socket has to be bound at least to a specific interface.
		 * This is done by explicitly binding the socket, or sending data through the socket.
		 *
		 * \param frame CAN frame structure to be send
		 * \param interface specifies the CAN interface used for reception
		 * \return number of bytes received
		 * \exception SocketException thrown if unable to receive datagram
		 */
		int receiveFrom( ::can_frame& frame, std::string& interface);

		/*!
		 * Read one CAN frame from this socket. If no interface has received a
		 * frame before the timeout runs out, the function will return
		 * without changing the buffer.
		 *
		 * In order to receive data, the socket has to be bound at least to a specific interface.
		 * This is done by explicitly binding the socket, or sending data through the socket.
		 *
		 * \param frame CAN frame structure to be send
		 * \param interface specifies the CAN interface used for reception
		 * \param timeout timeout in milliseconds
		 * \return number of bytes received, 0 on timeout
		 * \exception SocketException thrown if unable to receive datagram
		 */
		int timedReceiveFrom( ::can_frame& frame, std::string& interface, int timeout);

		/*!
		 * 
		 * \param filters Linux SocketCAN receive filters array
		 * \param len number of filters to set
		 * \exception SocketException
		 */
		void setReceiveFilter( const ::can_filter* filters, size_t len);

		/*!
		 * 
		 * \param filter Linux SocketCAN error-class mask
		 * \exception SocketException
		 */
		void setErrorFilter( ::can_err_mask_t filter);

		/*!
		 * 
		 * \param enable
		 * \exception SocketException
		 */
		void setLocalLoopback( bool enable);

		/*!
		 * 
		 * \param enable
		 * \exception SocketException
		 */
		void setReceiveOwnMessages( bool enable);
	};

} // namespace NET

#endif // NET_CANRawSocket_h__
