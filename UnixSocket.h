#ifndef NET_UnixSocket_h__
#define NET_UnixSocket_h__

#include "SimpleSocket.h"

namespace NET
{
	//! Unix Domain Socket
	class UnixSocket : public SimpleSocket
	{
	public:
		/*!
			Establish a socket connection with the given socket
			\param foreignPath the path of the remote socket
			\exception SocketException thrown if unable to establish connection
		*/
		void connect( const std::string& foreignPath);

		/*!
			Unset the socket path
			\exception SocketException thrown if unable to disconnect socket
		*/
		void disconnect();

		/*!
			Set the local path to the specified path
			\param localPath specifies where the socket should be bound
			\exception SocketException thrown if setting local path fails
		*/
		void bind( const std::string& localPath);

		/*!
			Get the local path
			\return local path of socket
			\exception SocketException thrown if fetch fails
		*/
		std::string getLocalPath();


		/*!
			Get the foreign path
			\return foreign path
			\exception SocketException thrown if fetch fails
		*/
		std::string getForeignPath();
	protected:
		UnixSocket( int type, int protocol);
	};

} // namespace NET

#endif // NET_UnixSocket_h__
