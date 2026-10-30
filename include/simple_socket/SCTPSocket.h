#ifndef NET_SCTPSocket_h__
#define NET_SCTPSocket_h__

#include "InternetSocket.h"

#include <netinet/in.h>
#include <netinet/sctp.h>
#include <vector>
#include <optional>

namespace NET
{
	//! SCTP socket class
	class SCTPSocket : public InternetSocket
	{
	public:
		enum abortFlag
		{
			ABORT = SCTP_ABORT,
			SHUTDOWN = SCTP_EOF,
			KEEPALIVE = 0
		};

		enum switchAddressFlag
		{
			KEEP_PRIMARY = 0,
			OVERRIDE_PRIMARY = SCTP_ADDR_OVER
		};

		enum receiveFlag
		{
		};

		SCTPSocket( uint16_t numOutStreams = 10, uint16_t maxInStreams = 65535, uint16_t maxAttempts = 4,
		            uint16_t maxInitTimeout = 0 /* TODO */);


		using InternetSocket::bind;
		int bind( const std::vector<std::string>& localAddresses, unsigned short localPort = 0);

		using InternetSocket::connect;
		int connect( const std::vector<std::string>& foreignAddresses, unsigned short foreignPort = 0);

		int state() const;
		int notAckedData() const;
		int pendingData() const;
		unsigned inStreams() const;
		unsigned outStreams() const;
		unsigned fragmentationPoint() const;
		std::string primaryAddress() const;

		using SimpleSocket::send;
		int send( const void* data, size_t length, uint16_t stream, unsigned ttl = 0, unsigned context = 0, unsigned ppid = 0,
		          abortFlag abort = KEEPALIVE, switchAddressFlag switchAddr = KEEP_PRIMARY);

		int sendUnordered( const void* data, size_t length, uint16_t stream, unsigned ttl = 0, unsigned context = 0,
		                   unsigned ppid = 0, abortFlag abort = KEEPALIVE, switchAddressFlag switchAddr = KEEP_PRIMARY);

		using SimpleSocket::receive;
		int receive( void* data, size_t maxLen, uint16_t& stream);
		int receive( void* data, size_t maxLen, uint16_t& stream, receiveFlag& flag);

		using SimpleSocket::timedReceive;
		int timedReceive( void* data, size_t maxLen, uint16_t& stream, int timeout);
		int timedReceive( void* data, size_t maxLen, uint16_t& stream, receiveFlag& flag, int timeout);

		//! Listen for incoming connections
		using SimpleSocket::listen;

		SCTPSocket accept() const;
		std::optional<SCTPSocket> timedAccept( int timeout) const;

	protected:
		void setInitValues( uint16_t ostr, uint16_t istr, uint16_t att, uint16_t time);

		// allow construction by accept()
		using InternetSocket::InternetSocket;
	};

} // namespace NET

#endif // NET_SCTPSocket_h__
