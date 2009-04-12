#ifndef NET_SCTPSocket_h_
#define NET_SCTPSocket_h_

#include "InternetSocket.h"

#include <netinet/in.h>
#include <netinet/sctp.h>

#include <vector>
#include <string>

namespace NET {
	class SCTPSocket : public InternetSocket {
	public:
		class Handle {
			Handle( int sock) : m_sockfd(sock) {}
			friend class SCTPSocket;
			int m_sockfd;
		public:
			operator bool () { return m_sockfd != 0; }
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
			blub
		};
	
		SCTPSocket( unsigned int numOutStreams = 10, unsigned int maxInStreams = 65535, unsigned int maxAttempts = 4, unsigned int maxInitTimeout = 0 /*???*/);
		SCTPSocket( const std::string& foreignAddress, unsigned short foreignPort, unsigned int numOutStreams = 10, unsigned int maxInStreams = 65535, unsigned int maxAttempts = 4, unsigned int maxInitTimeout = 0);
		SCTPSocket( const std::vector<std::string>& foreignAddresses, unsigned short foreignPort, unsigned int numOutStreams = 10, unsigned int maxInStreams = 65535, unsigned int maxAttempts = 4, unsigned int maxInitTimeout = 0);
		SCTPSocket( Handle handle);
		
		using InternetSocket::bind;
		int bind(const std::vector<std::string>& localAddresses, unsigned int port = 0);
	
		int state();
		int notAckedData();
		int pendingData();
		unsigned int inStreams();
		unsigned int outStreams();
		unsigned int fragmentationPoint();
		std::string primaryAddress();
		
		using SimpleSocket::send;
		int send( const void* data, int length, unsigned int stream, unsigned int ttl = 0, unsigned int context = 0, unsigned int ppid = 0, abortFlag abort = KEEPALIVE, switchAddressFlag switchAddr = KEEP_PRIMARY);
		int sendUnordered( const void* data, int length, unsigned int stream, unsigned int ttl = 0, unsigned int context = 0, unsigned int ppid = 0, abortFlag abort = KEEPALIVE, switchAddressFlag switchAddr = KEEP_PRIMARY);
		using SimpleSocket::receive;
		int receive( void* data, int maxLen, unsigned int& stream);
		int receive( void* data, int maxLen, unsigned int& stream, receiveFlag& flag);
		
		using InternetSocket::timedReceive;
		int timedReceive( void* data, int maxLen, unsigned int& stream, unsigned int timeout);
		int timedReceive( void* data, int maxLen, unsigned int& stream, receiveFlag& flag, unsigned int timeout);
		
		void listen( int backlog = 10);
		Handle accept();
	protected:
		void setInitValues( unsigned int ostr, unsigned int istr, unsigned int att, unsigned int time);
	};
}

#endif // NET_SCTPSocket_h_
