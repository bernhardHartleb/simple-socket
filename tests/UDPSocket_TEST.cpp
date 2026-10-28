#include <cppunit/extensions/HelperMacros.h>
#include <simple_socket/UDPSocket.h>

#include <cstring>
#include <string>

class UDPSocket_TEST : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(UDPSocket_TEST);
	CPPUNIT_TEST(testSendToReceiveFrom);
	CPPUNIT_TEST(testConnectedSendReceive);
	CPPUNIT_TEST(testTimedReceiveFrom);
	CPPUNIT_TEST(testZeroLengthDatagram);
	CPPUNIT_TEST(testTruncatedDatagram);
	CPPUNIT_TEST(testMulticast);
	CPPUNIT_TEST(testInvalidAddress);
	CPPUNIT_TEST_SUITE_END();

private:
	static constexpr const char* LOOPBACK = "127.0.0.1";

public:
	void testSendToReceiveFrom()
	{
		NET::UDPSocket sender;
		NET::UDPSocket receiver;

		sender.bind(LOOPBACK, 0);
		receiver.bind(LOOPBACK, 0);

		const unsigned short senderPort = sender.getLocalPort();
		const unsigned short receiverPort = receiver.getLocalPort();

		CPPUNIT_ASSERT(senderPort != 0);
		CPPUNIT_ASSERT(receiverPort != 0);

		const char message[] = "Hello UDP";
		sender.sendTo(message, sizeof(message), LOOPBACK, receiverPort);

		char buffer[sizeof(message)] = {};
		std::string sourceAddress;
		unsigned short sourcePort = 0;

		const int received = receiver.receiveFrom(buffer, sizeof(buffer), sourceAddress, sourcePort);

		CPPUNIT_ASSERT_EQUAL(static_cast<int>(sizeof(message)), received);
		CPPUNIT_ASSERT_EQUAL(std::string(LOOPBACK), sourceAddress);
		CPPUNIT_ASSERT_EQUAL(senderPort, sourcePort);
		CPPUNIT_ASSERT(std::memcmp(message, buffer, sizeof(message)) == 0);
	}

	void testConnectedSendReceive()
	{
		NET::UDPSocket client;
		NET::UDPSocket server;

		server.bind(LOOPBACK, 0);

		const unsigned short serverPort = server.getLocalPort();
		CPPUNIT_ASSERT(serverPort != 0);

		client.connect(LOOPBACK, serverPort);

		CPPUNIT_ASSERT_EQUAL(std::string(LOOPBACK), client.getForeignAddress());
		CPPUNIT_ASSERT_EQUAL(serverPort, client.getForeignPort());

		const char request[] = "UDP request";
		const int sent = client.send(request, sizeof(request));
		CPPUNIT_ASSERT_EQUAL(static_cast<int>(sizeof(request)), sent);
		CPPUNIT_ASSERT(!client.peerDisconnected());

		char buffer[64] = {};
		std::string sourceAddress;
		unsigned short sourcePort = 0;

		int received = server.receiveFrom(buffer, sizeof(buffer), sourceAddress, sourcePort);

		CPPUNIT_ASSERT_EQUAL(static_cast<int>(sizeof(request)), received);
		CPPUNIT_ASSERT_EQUAL(std::string(LOOPBACK), sourceAddress);
		CPPUNIT_ASSERT(sourcePort != 0);
		CPPUNIT_ASSERT(std::memcmp(request, buffer, sizeof(request)) == 0);

		const char reply[] = "UDP reply";
		server.sendTo(reply, sizeof(reply), sourceAddress, sourcePort);

		std::memset(buffer, 0, sizeof(buffer));
		received = client.timedReceive(buffer, sizeof(buffer), 1000);

		CPPUNIT_ASSERT_EQUAL(static_cast<int>(sizeof(reply)), received);
		CPPUNIT_ASSERT(std::memcmp(reply, buffer, sizeof(reply)) == 0);
	}

	void testTimedReceiveFrom()
	{
		NET::UDPSocket sender;
		NET::UDPSocket receiver;

		sender.bind(LOOPBACK, 0);
		receiver.bind(LOOPBACK, 0);

		const unsigned short receiverPort = receiver.getLocalPort();
		char buffer[32];
		std::memset(buffer, 0x5a, sizeof(buffer));
		std::string sourceAddress = "unchanged";
		unsigned short sourcePort = 12345;

		// Nothing has been sent yet
		int received = receiver.timedReceiveFrom(buffer, sizeof(buffer), sourceAddress, sourcePort, 50);

		CPPUNIT_ASSERT_EQUAL(0, received);
		CPPUNIT_ASSERT_EQUAL(std::string("unchanged"), sourceAddress);
		CPPUNIT_ASSERT_EQUAL(static_cast<unsigned short>(12345), sourcePort);
		CPPUNIT_ASSERT(!receiver.peerDisconnected());

		for (char c : buffer)
			CPPUNIT_ASSERT_EQUAL(static_cast<char>(0x5a), c);

		const char message[] = "Timed UDP";
		sender.sendTo(message, sizeof(message), LOOPBACK, receiverPort);

		received = receiver.timedReceiveFrom(buffer, sizeof(buffer), sourceAddress, sourcePort, 1000);

		CPPUNIT_ASSERT_EQUAL(static_cast<int>(sizeof(message)), received);
		CPPUNIT_ASSERT_EQUAL(std::string(LOOPBACK), sourceAddress);
		CPPUNIT_ASSERT_EQUAL(sender.getLocalPort(), sourcePort);
		CPPUNIT_ASSERT(std::memcmp(message, buffer, sizeof(message)) == 0);
	}

	void testZeroLengthDatagram()
	{
		NET::UDPSocket sender;
		NET::UDPSocket receiver;

		sender.bind(LOOPBACK, 0);
		receiver.bind(LOOPBACK, 0);

		const unsigned short receiverPort = receiver.getLocalPort();
		char dummy = 'X';
		sender.sendTo(&dummy, 0, LOOPBACK, receiverPort);

		std::string sourceAddress;
		unsigned short sourcePort = 0;

		const int received = receiver.receiveFrom(&dummy, sizeof(dummy), sourceAddress, sourcePort);

		// Zero is a valid UDP datagram size
		CPPUNIT_ASSERT_EQUAL(0, received);
		// receiveFrom() still provides the sender information, which proves
		// that an actual datagram was received rather than a timeout.
		CPPUNIT_ASSERT_EQUAL(std::string(LOOPBACK), sourceAddress);
		CPPUNIT_ASSERT_EQUAL(sender.getLocalPort(), sourcePort);
		CPPUNIT_ASSERT_EQUAL('X', dummy);
		CPPUNIT_ASSERT(!receiver.peerDisconnected());
	}

	void testTruncatedDatagram()
	{
		NET::UDPSocket sender;
		NET::UDPSocket receiver;

		sender.bind(LOOPBACK, 0);
		receiver.bind(LOOPBACK, 0);

		const unsigned short receiverPort = receiver.getLocalPort();
		const char message[] =
			"This datagram is deliberately larger than the receive buffer";

		sender.sendTo(message, sizeof(message), LOOPBACK, receiverPort);

		char buffer[8] = {};
		std::string sourceAddress;
		unsigned short sourcePort = 0;

		int received = receiver.receiveFrom(buffer, sizeof(buffer), sourceAddress, sourcePort);

		// Datagram sockets return only what fits into the supplied buffer
		CPPUNIT_ASSERT_EQUAL(static_cast<int>(sizeof(buffer)), received);
		CPPUNIT_ASSERT(std::memcmp(message, buffer, sizeof(buffer)) == 0);

		// Nothing from the truncated datagram should remain queued
		received = receiver.timedReceiveFrom(buffer, sizeof(buffer), sourceAddress, sourcePort, 50);

		CPPUNIT_ASSERT_EQUAL(0, received);
	}

	void testMulticast()
	{
		NET::UDPSocket sender;
		NET::UDPSocket receiver;

		const char group[] = "224.40.0.1";
		const char message[] = "The quick brown fox jumps over the lazy dog";
		const int len = sizeof(message);

		sender.setMulticastTTL(0);
		sender.connect(group, 47777);

		receiver.bind(47777);
		receiver.joinGroup(group);

		int sent = sender.send(message, len);
		CPPUNIT_ASSERT_EQUAL(len, sent);

		char buffer[64] = {};
		int received = receiver.receive(buffer, len);
		CPPUNIT_ASSERT_EQUAL(len, received);
		CPPUNIT_ASSERT(std::memcmp(message, buffer, len) == 0);

		receiver.leaveGroup(group);
		sent = sender.send(message, len);
		CPPUNIT_ASSERT_EQUAL(len, sent);

		received = receiver.timedReceive(buffer, len, 50);
		CPPUNIT_ASSERT_EQUAL(0, received);
	}

	void testInvalidAddress()
	{
		NET::UDPSocket socket_ipv4(NET::IPv4);
		NET::UDPSocket socket_ipv6(NET::IPv6);

		const std::string ipv4("127.0.0.1");
		const std::string ipv6("::1");

		const std::string bad_ip4("255.255.255.256");
		const std::string bad_ip6("2001:db8::1:1:1:1:1");
		const std::string bad_nul("");
		const std::string bad_len(255, '0');

		// IPv4
		CPPUNIT_ASSERT_THROW(socket_ipv4.bind(ipv6),    NET::SocketException);
		CPPUNIT_ASSERT_THROW(socket_ipv4.bind(bad_ip4), NET::SocketException);
		CPPUNIT_ASSERT_THROW(socket_ipv4.bind(bad_ip6), NET::SocketException);
		CPPUNIT_ASSERT_THROW(socket_ipv4.bind(bad_nul), NET::SocketException);
		CPPUNIT_ASSERT_THROW(socket_ipv4.bind(bad_len), NET::SocketException);

		// IPv6
		CPPUNIT_ASSERT_THROW(socket_ipv6.bind(ipv4),    NET::SocketException);
		CPPUNIT_ASSERT_THROW(socket_ipv6.bind(bad_ip4), NET::SocketException);
		CPPUNIT_ASSERT_THROW(socket_ipv6.bind(bad_ip6), NET::SocketException);
		CPPUNIT_ASSERT_THROW(socket_ipv6.bind(bad_nul), NET::SocketException);
		CPPUNIT_ASSERT_THROW(socket_ipv6.bind(bad_len), NET::SocketException);

		// this should work
		socket_ipv4.bind(ipv4);
		socket_ipv6.bind(ipv6);
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION(UDPSocket_TEST);
