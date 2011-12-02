#ifndef NET_CANRawSocket_h__
#define NET_CANRawSocket_h__

#include "CANSocket.h"
#include "CANFrame.h"

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
		 * \param buffer pointer to CAN frame structure to be send
		 * \param len size of the can frame structure
		 * \param interface specifies the CAN interface to send used for sending
		 * \exception SocketException thrown if unable to send frame
		 */
		void sendTo( const void* buffer, size_t len, const std::string& interface);

		/*!
		 * Read one CAN frame from this socket.
		 *
		 * In order to receive data, the socket has to be bound at least to a specific interface.
		 * This is done by explicitly binding the socket, or sending data through the socket.
		 *
		 * \param buffer pointer to CAN frame structure
		 * \param len size of the can frame structure
		 * \param interface specifies the CAN interface used for reception
		 * \return number of bytes received
		 * \exception SocketException thrown if unable to receive datagram
		 */
		int receiveFrom( void* buffer, size_t len, std::string& interface);

		/*!
		 * Read one CAN frame from this socket. If no interface has received a
		 * frame before the timeout runs out, the function will return
		 * without changing the buffer.
		 *
		 * In order to receive data, the socket has to be bound at least to a specific interface.
		 * This is done by explicitly binding the socket, or sending data through the socket.
		 *
		 * \param buffer pointer to CAN frame structure
		 * \param len size of the can frame structure
		 * \param interface specifies the CAN interface used for reception
		 * \param timeout timeout in milliseconds
		 * \return number of bytes received, 0 on timeout
		 * \exception SocketException thrown if unable to receive datagram
		 */
		int timedReceiveFrom( void* buffer, size_t len, std::string& interface, int timeout);

		/*!
		 * 
		 * \param filters
		 * \param len number of filters to set
		 * \exception SocketException
		 */
		void setReceiveFilter( const can_filter* filters, size_t len);

		/*!
		 * 
		 * \param filters
		 * \param len number of filters to set
		 * \exception SocketException
		 */
		void setErrorFilter( const can_filter* filters, size_t len);

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
