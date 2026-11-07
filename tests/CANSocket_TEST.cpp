#include <cppunit/extensions/HelperMacros.h>
#include <simple_socket/CANRawSocket.h>

#include <net/if.h>
#include <algorithm>
#include <initializer_list>

using NET::IOResult;
using NET::IOStatus;

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
		IOResult ret = sender.sendTo(sent, INTERFACE);
		CPPUNIT_ASSERT(ret);

		::can_frame received{};
		std::string sourceInterface;
		ret = receiver.timedReceiveFrom(received, sourceInterface, 1000);

		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, ret.status);
		CPPUNIT_ASSERT_EQUAL(sizeof(::can_frame), ret.bytes);
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
		IOResult ret = sender.send(sent);
		CPPUNIT_ASSERT(ret);

		::can_frame received{};
		ret = receiver.timedReceive(received, 1000);

		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, ret.status);
		CPPUNIT_ASSERT_EQUAL(sizeof(::can_frame), ret.bytes);
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
		IOResult ret = receiver.timedReceive(frame, 50);

		CPPUNIT_ASSERT_EQUAL(IOStatus::Timeout, ret.status);
		CPPUNIT_ASSERT_EQUAL(0ul, ret.bytes);
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

		IOResult ret = sender.send(rejected);
		CPPUNIT_ASSERT(ret);

		::can_frame received{};
		// The non-matching frame must not reach this socket
		ret = receiver.timedReceive(received, 50);
		CPPUNIT_ASSERT_EQUAL(IOStatus::Timeout, ret.status);
		CPPUNIT_ASSERT_EQUAL(0ul, ret.bytes);

		ret = sender.send(accepted);
		CPPUNIT_ASSERT(ret);

		ret = receiver.timedReceive(received, 1000);
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, ret.status);
		CPPUNIT_ASSERT_EQUAL(sizeof(::can_frame), ret.bytes);
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
		IOResult ret = socket.send(sent);
		CPPUNIT_ASSERT(ret);

		ret = socket.timedReceive(received, 50);
		CPPUNIT_ASSERT_EQUAL(IOStatus::Timeout, ret.status);
		CPPUNIT_ASSERT_EQUAL(0ul, ret.bytes);

		socket.setReceiveOwnMessages(true);
		ret = socket.send(sent);
		CPPUNIT_ASSERT(ret);

		ret = socket.timedReceive(received, 1000);
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, ret.status);
		CPPUNIT_ASSERT_EQUAL(sizeof(::can_frame), ret.bytes);
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
			CPPUNIT_ASSERT_THROW(auto t = socket.send(frame), NET::SocketException);
		}
		{
			NET::CANRawSocket socket;
			CPPUNIT_ASSERT_THROW(auto t = socket.sendTo(frame, "no-such-can-interface"), NET::SocketException);
		}
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION(CANSocket_TEST);
