#include <cppunit/extensions/HelperMacros.h>
#include <UDPSocket.h>

#include <cstring>

static const char send_msg[] = "The quick brown fox jumps over the lazy dog";
static char recv_msg[sizeof(send_msg)];
static const int len = sizeof(send_msg);

class UDPSocket_TEST : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE( UDPSocket_TEST );
	CPPUNIT_TEST( testPeerStatus );
	CPPUNIT_TEST( testMulticast );
	CPPUNIT_TEST( testSendTo );
	CPPUNIT_TEST_SUITE_END();

private:
	NET::UDPSocket* send_socket;
	NET::UDPSocket* recv_socket;

public:
	void setUp()
	{
		send_socket = new NET::UDPSocket();
		recv_socket = new NET::UDPSocket();
	}

	void tearDown()
	{
		delete send_socket;
		delete recv_socket;
	}

	void testPeerStatus()
	{
		send_socket->connect( "127.0.0.1", 65001);
		CPPUNIT_ASSERT( !send_socket->peerDisconnected() );
		send_socket->send( "", 1); // ok, no icmp received yet
		send_socket->send( "", 1); // icmp received, send fails
		CPPUNIT_ASSERT( send_socket->peerDisconnected() );
	}

	void testMulticast()
	{
		int ret;
		send_socket->setMulticastTTL(0);
		send_socket->connect( "224.40.0.1", 47777);
		recv_socket->bind(47777);
		recv_socket->joinGroup("224.40.0.1");

		ret = send_socket->send( send_msg, len);
		CPPUNIT_ASSERT_EQUAL( len, ret );
		ret = recv_socket->receive( recv_msg, len);
		CPPUNIT_ASSERT_EQUAL( len, ret );
		CPPUNIT_ASSERT( std::memcmp( send_msg, recv_msg, len) == 0 );

		recv_socket->leaveGroup("224.40.0.1");
		ret = send_socket->send( send_msg, len);
		CPPUNIT_ASSERT_EQUAL( len, ret );

		ret = recv_socket->timedReceive( recv_msg, len, 1);
		CPPUNIT_ASSERT_EQUAL( 0, ret );
	}

	void testSendTo()
	{
		int ret;
		std::string source;
		unsigned short port = 0;
		send_socket->bind(47776);
		recv_socket->bind(47777);
		send_socket->sendTo( send_msg, len, "127.0.0.1", 47777);

		ret = recv_socket->receiveFrom( recv_msg, len, source, port);
		CPPUNIT_ASSERT_EQUAL( len, ret );
		CPPUNIT_ASSERT_EQUAL( (unsigned short)47776, port );
		CPPUNIT_ASSERT_EQUAL( std::string("127.0.0.1"), source );

		ret = recv_socket->timedReceiveFrom( recv_msg, len, source, port, 10);
		CPPUNIT_ASSERT_EQUAL( 0, ret );
		CPPUNIT_ASSERT_EQUAL( (unsigned short)47776, port );
		CPPUNIT_ASSERT_EQUAL( std::string("127.0.0.1"), source );
		CPPUNIT_ASSERT( std::memcmp( send_msg, recv_msg, len) == 0 );
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( UDPSocket_TEST );
