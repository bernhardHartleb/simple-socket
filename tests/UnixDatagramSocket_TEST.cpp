#include <cppunit/extensions/HelperMacros.h>
#include "../UnixDatagramSocket.h"

#include <unistd.h>
#include <cstring>

static const char sock_file[] = "/tmp/simple-socket_test.sock";
static const char send_msg[] = "The quick brown fox jumps over the lazy dog";
static char recv_msg[sizeof(send_msg)];
static const int len = sizeof(send_msg);

class UnixDatagramSocket_TEST : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE( UnixDatagramSocket_TEST );
	CPPUNIT_TEST( testSendTo );
	CPPUNIT_TEST_SUITE_END();

private:
	NET::UnixDatagramSocket* send_socket;
	NET::UnixDatagramSocket* recv_socket;

public:
	void setUp()
	{
		send_socket = new NET::UnixDatagramSocket();
		recv_socket = new NET::UnixDatagramSocket();
	}

	void tearDown()
	{
		delete send_socket;
		delete recv_socket;

		// remove this if socket can handle it
		::unlink(sock_file);
	}

	void testSendTo()
	{
		int ret;
		std::string source;
		send_socket->bind(sock_file);
		recv_socket->bind(sock_file);
		send_socket->sendTo( send_msg, len, sock_file);

		ret = recv_socket->receiveFrom( recv_msg, len, source);
		CPPUNIT_ASSERT_EQUAL( len, ret );
		CPPUNIT_ASSERT_EQUAL( std::string(sock_file), source );

		ret = recv_socket->timedReceiveFrom( recv_msg, len, source, 10);
		CPPUNIT_ASSERT_EQUAL( 0, ret );
		CPPUNIT_ASSERT_EQUAL( std::string(sock_file), source );
		CPPUNIT_ASSERT( std::memcmp(send_msg, recv_msg, len) == 0 );
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( UnixDatagramSocket_TEST );
