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
		class Handle
		{
		public:
			friend class SCTPSocket;
			operator bool () { return m_sockfd != 0; }
		private:
			Handle( int sock) : m_sockfd(sock) {}
			int m_sockfd;
		};

		enum abortFlag {
			ABORT = SCTP_ABORT,
			SHUTDOWN = SCTP_EOF,
			KEEPALIVE = 0
		};

		enum switchAddressFlag {
			KEEP_PRIMARY = 0,
			OVERRIDE_PRIMARY = SCTP_ADDR_OVER
		};

		enum receiveFlag {
			TODO
		};

		SCTPSocket( unsigned numOutStreams = 10, unsigned maxInStreams = 65535, unsigned maxAttempts = 4,
			    unsigned maxInitTimeout = 0 /*???*/);

		SCTPSocket( const std::string& foreignAddress, unsigned short foreignPort, unsigned numOutStreams = 10,
			    unsigned maxInStreams = 65535, unsigned maxAttempts = 4, unsigned maxInitTimeout = 0);

		SCTPSocket( const std::vector<std::string>& foreignAddresses, unsigned short foreignPort, unsigned numOutStreams = 10,
			    unsigned maxInStreams = 65535, unsigned maxAttempts = 4, unsigned maxInitTimeout = 0);

		SCTPSocket( Handle handle);

		using InternetSocket::bind;
		int bind( const std::vector<std::string>& localAddresses, unsigned short port = 0);

		int state();
		int notAckedData();
		int pendingData();
		unsigned inStreams();
		unsigned outStreams();
		unsigned fragmentationPoint();
		std::string primaryAddress();

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
		Handle accept();
		Handle timedAccept( unsigned timeout);

		static Handle emptyHandle();
	protected:
		void setInitValues( unsigned ostr, unsigned istr, unsigned att, unsigned time);
	};

} // namespace NET

#endif // NET_SCTPSocket_h_
