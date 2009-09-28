#ifndef NET_SCTPSocket_h_
#define NET_SCTPSocket_h_

#include "InternetSocket.h"

#include <netinet/in.h>
#include <netinet/sctp.h>

#include <vector>
#include <string>

namespace NET
{
	//! put docu here
	class SCTPSocket : public InternetSocket
	{
	public:
		//! handle for a new socket returned by accept
		class Handle
		{
		public:
			Handle() : m_sockfd(0) {}
			operator bool() const { return m_sockfd != 0; }
		private:
			friend class SCTPSocket;
			explicit Handle( int sock) : m_sockfd(sock) {}
			int m_sockfd;
		};

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
			TODO
		};

		SCTPSocket( unsigned numOutStreams = 10, unsigned maxInStreams = 65535, unsigned maxAttempts = 4,
		            unsigned maxInitTimeout = 0 /*???*/);

		SCTPSocket( Handle handle);

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
		int send( const void* data, int length, unsigned stream, unsigned ttl = 0, unsigned context = 0, unsigned ppid = 0,
		          abortFlag abort = KEEPALIVE, switchAddressFlag switchAddr = KEEP_PRIMARY);

		int sendUnordered( const void* data, int length, unsigned stream, unsigned ttl = 0, unsigned context = 0,
		                   unsigned ppid = 0, abortFlag abort = KEEPALIVE, switchAddressFlag switchAddr = KEEP_PRIMARY);

		using SimpleSocket::receive;
		int receive( void* data, int maxLen, unsigned& stream);
		int receive( void* data, int maxLen, unsigned& stream, receiveFlag& flag);

		using SimpleSocket::timedReceive;
		int timedReceive( void* data, int maxLen, unsigned& stream, unsigned timeout);
		int timedReceive( void* data, int maxLen, unsigned& stream, receiveFlag& flag, unsigned timeout);

		void listen( int backlog = 10);
		Handle accept() const;
		Handle timedAccept( unsigned timeout) const;

	protected:
		void setInitValues( unsigned ostr, unsigned istr, unsigned att, unsigned time);
	};

} // namespace NET

#endif // NET_SCTPSocket_h_
