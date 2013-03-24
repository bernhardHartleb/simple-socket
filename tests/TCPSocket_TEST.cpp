#include <cppunit/extensions/HelperMacros.h>
#include <../TCPSocket.h>

#include <cstring>

static const char send_msg[] = "The quick brown fox jumps over the lazy dog";
static char recv_msg[sizeof(send_msg)];
static const int len = sizeof(send_msg);

class TCPSocket_TEST : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE( TCPSocket_TEST );
	CPPUNIT_TEST( testSocketHandle );
	CPPUNIT_TEST( testPeerStatus );
	CPPUNIT_TEST_SUITE_END();

private:
	NET::TCPSocket* server_socket;
	NET::TCPSocket* client_socket;

public:
	void setUp()
	{
		server_socket = new NET::TCPSocket();
		client_socket = new NET::TCPSocket();
	}

	void tearDown()
	{
		delete server_socket;
		delete client_socket;
	}

	void testSocketHandle()
	{
		server_socket->bind( "127.0.0.1", 47777);
		server_socket->listen();
		client_socket->connect( "127.0.0.1", 47777);

		NET::TCPSocket::Handle tmp_handle;
		tmp_handle = server_socket->accept();
		NET::TCPSocket::Handle handle;
		NET::TCPSocket::Handle& ref_handle = handle;
		handle = ref_handle;
		CPPUNIT_ASSERT( !handle );
		CPPUNIT_ASSERT( tmp_handle );
		handle = tmp_handle;
		CPPUNIT_ASSERT( handle );
		CPPUNIT_ASSERT( !tmp_handle );
		NET::TCPSocket session_socket(handle);
		CPPUNIT_ASSERT( !handle );
		CPPUNIT_ASSERT_THROW( NET::TCPSocket bad_socket(handle), NET::SocketException );
		client_socket->disconnect();
	}

	void testPeerStatus()
	{
		int ret;
		server_socket->bind( "127.0.0.1", 47777);
		server_socket->listen();
		CPPUNIT_ASSERT( !server_socket->peerDisconnected() );

		client_socket->connect( "127.0.0.1", 47777);
		NET::TCPSocket::Handle handle = server_socket->accept();
		CPPUNIT_ASSERT( handle );
		NET::TCPSocket session_socket(handle);
		CPPUNIT_ASSERT( !handle );

		session_socket.send( send_msg, len);
		ret = client_socket->receive( recv_msg, len);
		CPPUNIT_ASSERT_EQUAL( len, ret );
		CPPUNIT_ASSERT( !session_socket.peerDisconnected() );
		CPPUNIT_ASSERT( std::memcmp( send_msg, recv_msg, len) == 0 );

		client_socket->disconnect();
		ret = session_socket.send( send_msg, len);
		CPPUNIT_ASSERT_EQUAL( -1, ret );
		CPPUNIT_ASSERT( session_socket.peerDisconnected() );
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( TCPSocket_TEST );
