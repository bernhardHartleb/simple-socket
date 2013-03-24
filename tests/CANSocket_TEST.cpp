#include <cppunit/extensions/HelperMacros.h>
#include <../CANRawSocket.h>
#include <../CANFrame.h>

#include <stdexcept>
#include <cstring>

static NET::can_frame send_msg;
static NET::can_frame recv_msg;
static const int len = sizeof(NET::can_frame);

class CANSocket_TEST : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE( CANSocket_TEST );
	CPPUNIT_TEST( testFilter );
	CPPUNIT_TEST( testSendTo );
	CPPUNIT_TEST_SUITE_END();

private:
	NET::CANRawSocket* send_socket;
	NET::CANRawSocket* recv_socket;

public:
	void setUp()
	{
		NET::can_frame frame1;
		frame1.id.value = 0x10000000;
		frame1.id.ext_format = 1;
		frame1.dlc = 2;
		frame1[0] = 0x00;
		frame1[1] = 0x01;
		
		frame1.data[0] = 0x00;
		frame1.data[0] = 0x01;

		send_socket = new NET::CANRawSocket();
		recv_socket = new NET::CANRawSocket();
	}

	void tearDown()
	{
		delete send_socket;
		delete recv_socket;
	}

	void testFilter()
	{
		NET::can_filter filter[2];
		filter[0].id.value = 0x1ff;
		filter[0].id.ext_format = 0;
		filter[0].mask.value = 0x1ff;
		filter[0].mask.ext_format = 1;

		recv_socket->setReceiveFilter( filter, 2);
	}

	void testSendTo()
	{
		int ret;
		std::string source;
		send_socket->bind("vcan0");
		recv_socket->bind("vcan0");
		send_socket->sendTo( send_msg, len, "vcan0");

		ret = recv_socket->receiveFrom( recv_msg, len, source);
		CPPUNIT_ASSERT_EQUAL( len, ret );
		CPPUNIT_ASSERT_EQUAL( std::string("vcan0"), source );

		ret = recv_socket->timedReceiveFrom( recv_msg, len, source, 10);		
		CPPUNIT_ASSERT_EQUAL( 0, ret );
		CPPUNIT_ASSERT_EQUAL( std::string("vcan0"), source );
		CPPUNIT_ASSERT( std::memcmp(send_msg, recv_msg, len) == 0 );
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( CANSocket_TEST );
