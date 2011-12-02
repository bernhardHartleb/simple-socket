#ifndef NET_CANSocket_h__
#define NET_CANSocket_h__

#include "SimpleSocket.h"

struct sockaddr_can;

namespace NET
{
	//! CAN socket class
	class CANSocket : public SimpleSocket
	{
	public:
		/*!
		 * Establish a socket connection with the given socket
		 * \param interface specifies the CAN interface to connect to
		 * \exception SocketException thrown if unable to establish connection
		 */
		void connect( const std::string& interface = "");

		/*!
		 * Set the local interface to the specified interface
		 * \param interface specifies the CAN interface to bind to
		 * \exception SocketException thrown if setting local path fails
		 */
		void bind( const std::string& interface = "");

		/*!
		 * Get the local interface (after binding the socket)
		 * \return local interface of socket
		 * \exception SocketException thrown if fetch fails
		 */
		std::string getLocalInterface() const;

		/*!
		 * Get the foreign interface. Call connect() before using this function.
		 * \return foreign interface
		 * \exception SocketException thrown if fetch fails
		 */
		std::string getForeignInterface() const;

	protected:
		//! allows a subclass to create new socket
		CANSocket( int type, int protocol);

		//! get CAN interface name from socket address structure
		std::string getInterfaceName( const sockaddr_can& addr) const;

		//! get CAN interface index from interface name
		int getInterfaceIndex( const std::string& interface) const;
	};

} // namespace NET

#endif // NET_CANSocket_h__
