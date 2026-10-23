#include <cppunit/extensions/HelperMacros.h>
#include "../CANRawSocket.h"

#include <stdexcept>
#include <cstring>

static ::can_frame send_msg;
static ::can_frame recv_msg;
static const int len = sizeof(::can_frame);

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
		::can_frame frame1;
		frame1.can_id = (0x10000000 & CAN_EFF_MASK) | CAN_EFF_FLAG;
		frame1.len = 2;
		frame1.data[0] = 0x00;
		frame1.data[1] = 0x01;

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
		::can_filter filter[2];
		filter[0].can_id = 0x1ff;
		filter[0].can_mask = CAN_SFF_MASK;

		recv_socket->setReceiveFilter( filter, 2);
	}

	void testSendTo()
	{
		int ret;
		std::string source;
		send_socket->bind("vcan0");
		recv_socket->bind("vcan0");
		send_socket->sendTo( send_msg, "vcan0");

		ret = recv_socket->receiveFrom( recv_msg, source);
		CPPUNIT_ASSERT_EQUAL( ret, len );
		CPPUNIT_ASSERT_EQUAL( std::string("vcan0"), source );

		ret = recv_socket->timedReceiveFrom( recv_msg, source, 10);
		CPPUNIT_ASSERT_EQUAL( 0, ret );
		CPPUNIT_ASSERT_EQUAL( std::string("vcan0"), source );
		CPPUNIT_ASSERT( std::memcmp(&send_msg, &recv_msg, len) == 0 );
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( CANSocket_TEST );
