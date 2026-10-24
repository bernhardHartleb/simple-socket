#include <cppunit/extensions/HelperMacros.h>
#include "../CANRawSocket.h"

#include <net/if.h>
#include <algorithm>
#include <initializer_list>

class CANSocket_TEST : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(CANSocket_TEST);
	CPPUNIT_TEST(testSendToReceiveFrom);
	CPPUNIT_TEST(testBoundSendReceive);
	CPPUNIT_TEST(testTimedReceive);
	CPPUNIT_TEST(testReceiveFilter);
	CPPUNIT_TEST(testReceiveOwnMessages);
	CPPUNIT_TEST(testInvalidInterface);
	CPPUNIT_TEST_SUITE_END();

private:
	static constexpr const char* INTERFACE = "vcan0";

	static ::can_frame makeFrame(::canid_t canId, std::initializer_list<unsigned char> data)
	{
		::can_frame frame{};
		frame.can_id = canId;
		frame.len = static_cast<decltype(frame.len)>(data.size());

		std::copy(data.begin(), data.end(), frame.data);
		return frame;
	}

	static void assertFrameEqual(const ::can_frame& expected, const ::can_frame& actual)
	{
		CPPUNIT_ASSERT_EQUAL(expected.can_id, actual.can_id);
		CPPUNIT_ASSERT_EQUAL(static_cast<unsigned int>(expected.len), static_cast<unsigned int>(actual.len));

		for (unsigned int i = 0; i < expected.len; ++i)
			CPPUNIT_ASSERT_EQUAL(
				static_cast<unsigned int>(expected.data[i]),
				static_cast<unsigned int>(actual.data[i]));
	}

	static ::can_filter exactStandardFilter(::canid_t canId)
	{
		::can_filter filter{};
		filter.can_id = canId;
		// Match the exact 11-bit CAN ID and also require a normal
		// standard data frame rather than EFF/RTR.
		filter.can_mask = CAN_SFF_MASK | CAN_EFF_FLAG | CAN_RTR_FLAG;

		return filter;
	}

public:
	void testSendToReceiveFrom()
	{
		NET::CANRawSocket sender;
		NET::CANRawSocket receiver;

		sender.bind(INTERFACE);
		receiver.bind(INTERFACE);

		CPPUNIT_ASSERT_EQUAL(std::string(INTERFACE), sender.getLocalInterface());
		CPPUNIT_ASSERT_EQUAL(std::string(INTERFACE), receiver.getLocalInterface());

		const ::canid_t extid = (0x10000000 & CAN_EFF_MASK) | CAN_EFF_FLAG;
		const ::can_frame sent = makeFrame(extid, {0x10, 0x20, 0x30, 0x40});
		sender.sendTo(sent, INTERFACE);

		::can_frame received{};
		std::string sourceInterface;
		const int ret = receiver.timedReceiveFrom(received, sourceInterface, 1000);

		CPPUNIT_ASSERT_EQUAL(static_cast<int>(sizeof(::can_frame)), ret);
		CPPUNIT_ASSERT_EQUAL(std::string(INTERFACE), sourceInterface);
		assertFrameEqual(sent, received);
	}

	void testBoundSendReceive()
	{
		NET::CANRawSocket sender;
		NET::CANRawSocket receiver;

		sender.bind(INTERFACE);
		receiver.bind(INTERFACE);

		const ::can_frame sent = makeFrame(0x234, {0x01, 0x02, 0x03});

		// A bound CAN_RAW socket can use ordinary send()
		// because the bound interface supplies the destination.
		sender.send(sent);

		::can_frame received{};
		const int ret = receiver.timedReceive(received, 1000);

		CPPUNIT_ASSERT_EQUAL(static_cast<int>(sizeof(::can_frame)), ret);
		assertFrameEqual(sent, received);
	}

	void testTimedReceive()
	{
		NET::CANRawSocket receiver;

		const ::can_filter filter = exactStandardFilter(0x6A1);
		receiver.setReceiveFilter(&filter, 1);
		receiver.bind(INTERFACE);

		::can_frame frame = makeFrame(0x777, {0xAA});
		const ::can_frame original = frame;
		const int ret = receiver.timedReceive(frame, 50);

		CPPUNIT_ASSERT_EQUAL(0, ret);
		// Timeout must not modify the receive buffer
		assertFrameEqual(original, frame);
	}

	void testReceiveFilter()
	{
		NET::CANRawSocket sender;
		NET::CANRawSocket receiver;

		const ::can_filter filter = exactStandardFilter(0x321);
		receiver.setReceiveFilter(&filter, 1);
		receiver.bind(INTERFACE);
		sender.bind(INTERFACE);

		const ::can_frame rejected = makeFrame(0x320, {0x11});
		const ::can_frame accepted = makeFrame(0x321, {0x22, 0x33});

		sender.send(rejected);

		::can_frame received{};
		// The non-matching frame must not reach this socket
		int ret = receiver.timedReceive(received, 50);
		CPPUNIT_ASSERT_EQUAL(0, ret);

		sender.send(accepted);
		ret = receiver.timedReceive(received, 1000);

		CPPUNIT_ASSERT_EQUAL(static_cast<int>(sizeof(::can_frame)), ret);
		assertFrameEqual(accepted, received);
	}


	void testReceiveOwnMessages()
	{
		NET::CANRawSocket socket;

		const ::can_filter filter = exactStandardFilter(0x456);
		socket.setReceiveFilter(&filter, 1);
		socket.bind(INTERFACE);

		const ::can_frame sent = makeFrame(0x456, {0xCA, 0xFE});
		::can_frame received{};

		// SocketCAN local loopback is enabled by default, but a RAW
		// socket does not receive its own transmitted frames by default.
		socket.send(sent);

		int ret = socket.timedReceive(received, 50);
		CPPUNIT_ASSERT_EQUAL(0, ret);

		socket.setReceiveOwnMessages(true);
		socket.send(sent);

		ret = socket.timedReceive(received, 1000);

		CPPUNIT_ASSERT_EQUAL(static_cast<int>(sizeof(::can_frame)), ret);
		assertFrameEqual(sent, received);
	}

	void testInvalidInterface()
	{
		const ::can_frame frame = makeFrame(0x123, {0x01});

		{
			NET::CANRawSocket socket;
			CPPUNIT_ASSERT_THROW(socket.bind("no-such-can-interface"), NET::SocketException);
		}

		{
			NET::CANRawSocket socket;
			const std::string tooLong(IF_NAMESIZE, 'x');
			CPPUNIT_ASSERT_THROW(socket.bind(tooLong), NET::SocketException);
		}

		{
			NET::CANRawSocket socket;
			// send() has no interface to use until the RAW socket is bound
			CPPUNIT_ASSERT_THROW(socket.send(frame), NET::SocketException);
		}

		{
			NET::CANRawSocket socket;
			CPPUNIT_ASSERT_THROW(socket.sendTo(frame, "no-such-can-interface"), NET::SocketException);
		}
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION(CANSocket_TEST);
