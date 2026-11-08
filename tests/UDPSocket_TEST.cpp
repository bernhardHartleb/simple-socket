#include <cppunit/extensions/HelperMacros.h>
#include <simple_socket/UDPSocket.h>
#include <simple_socket/TCPSocket.h>

#include <cstring>
#include <string>

using NET::IOResult;
using NET::IOStatus;
using NET::ConnectResult;

class UDPSocket_TEST : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(UDPSocket_TEST);
	CPPUNIT_TEST(testSendToReceiveFrom);
	CPPUNIT_TEST(testConnectedSendReceive);
	CPPUNIT_TEST(testTimedReceiveFrom);
	CPPUNIT_TEST(testZeroLengthDatagram);
	CPPUNIT_TEST(testTruncatedDatagram);
	CPPUNIT_TEST(testNonBlockingReceiveFrom);
	CPPUNIT_TEST(testNonBlockingZeroLengthDatagram);
	CPPUNIT_TEST(testNonBlockingConnectedReceive);
	CPPUNIT_TEST(testMulticast);
	CPPUNIT_TEST(testInvalidAddress);
	CPPUNIT_TEST(testAdoptNativeHandle);
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
		IOResult sent = sender.sendTo(message, sizeof(message), LOOPBACK, receiverPort);
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, sent.status);
		CPPUNIT_ASSERT_EQUAL(sizeof(message), sent.bytes);

		char buffer[sizeof(message)] = {};
		std::string sourceAddress;
		unsigned short sourcePort = 0;

		IOResult recv = receiver.receiveFrom(buffer, sizeof(buffer), sourceAddress, sourcePort);
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, recv.status);
		CPPUNIT_ASSERT_EQUAL(sizeof(message), recv.bytes);
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

		ConnectResult res = client.connect(LOOPBACK, serverPort);
		CPPUNIT_ASSERT_EQUAL(ConnectResult::Connected, res);
		CPPUNIT_ASSERT_EQUAL(std::string(LOOPBACK), client.getForeignAddress());
		CPPUNIT_ASSERT_EQUAL(serverPort, client.getForeignPort());

		const char request[] = "UDP request";
		IOResult sent = client.send(request, sizeof(request));
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, sent.status);
		CPPUNIT_ASSERT_EQUAL(sizeof(request), sent.bytes);
		CPPUNIT_ASSERT(!client.peerDisconnected());

		char buffer[64] = {};
		std::string sourceAddress;
		unsigned short sourcePort = 0;

		IOResult recv = server.receiveFrom(buffer, sizeof(buffer), sourceAddress, sourcePort);
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, recv.status);
		CPPUNIT_ASSERT_EQUAL(sizeof(request), recv.bytes);
		CPPUNIT_ASSERT_EQUAL(std::string(LOOPBACK), sourceAddress);
		CPPUNIT_ASSERT(sourcePort != 0);
		CPPUNIT_ASSERT(std::memcmp(request, buffer, sizeof(request)) == 0);

		const char reply[] = "UDP reply";
		sent = server.sendTo(reply, sizeof(reply), sourceAddress, sourcePort);
		CPPUNIT_ASSERT(sent);

		std::memset(buffer, 0, sizeof(buffer));
		recv = client.timedReceive(buffer, sizeof(buffer), 1000);
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, recv.status);
		CPPUNIT_ASSERT_EQUAL(sizeof(reply), recv.bytes);
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
		IOResult recv = receiver.timedReceiveFrom(buffer, sizeof(buffer), sourceAddress, sourcePort, 50);
		CPPUNIT_ASSERT_EQUAL(IOStatus::Timeout, recv.status);
		CPPUNIT_ASSERT_EQUAL(0ul, recv.bytes);
		CPPUNIT_ASSERT_EQUAL(std::string("unchanged"), sourceAddress);
		CPPUNIT_ASSERT_EQUAL(static_cast<unsigned short>(12345), sourcePort);
		CPPUNIT_ASSERT(!receiver.peerDisconnected());

		for (char c : buffer)
			CPPUNIT_ASSERT_EQUAL(static_cast<char>(0x5a), c);

		const char message[] = "Timed UDP";
		IOResult sent = sender.sendTo(message, sizeof(message), LOOPBACK, receiverPort);
		CPPUNIT_ASSERT(sent);

		recv = receiver.timedReceiveFrom(buffer, sizeof(buffer), sourceAddress, sourcePort, 1000);
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, recv.status);
		CPPUNIT_ASSERT_EQUAL(sizeof(message), recv.bytes);
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
		IOResult sent = sender.sendTo(&dummy, 0, LOOPBACK, receiverPort);
		CPPUNIT_ASSERT(sent);

		std::string sourceAddress;
		unsigned short sourcePort = 0;

		IOResult recv = receiver.receiveFrom(&dummy, sizeof(dummy), sourceAddress, sourcePort);
		// Zero is a valid UDP datagram size
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, recv.status);
		CPPUNIT_ASSERT_EQUAL(0ul, recv.bytes);
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

		IOResult sent = sender.sendTo(message, sizeof(message), LOOPBACK, receiverPort);
		CPPUNIT_ASSERT(sent);

		char buffer[8] = {};
		std::string sourceAddress;
		unsigned short sourcePort = 0;

		IOResult recv = receiver.receiveFrom(buffer, sizeof(buffer), sourceAddress, sourcePort);
		// Datagram sockets return only what fits into the supplied buffer
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, recv.status);
		CPPUNIT_ASSERT_EQUAL(sizeof(buffer), recv.bytes);
		CPPUNIT_ASSERT(std::memcmp(message, buffer, sizeof(buffer)) == 0);

		recv = receiver.timedReceiveFrom(buffer, sizeof(buffer), sourceAddress, sourcePort, 50);
		// Nothing from the truncated datagram should remain queued
		CPPUNIT_ASSERT_EQUAL(IOStatus::Timeout, recv.status);
		CPPUNIT_ASSERT_EQUAL(0ul, recv.bytes);
	}

	void testMulticast()
	{
		NET::UDPSocket sender;
		NET::UDPSocket receiver;

		const char group[] = "224.40.0.1";
		const char message[] = "The quick brown fox jumps over the lazy dog";
		const size_t len = sizeof(message);

		sender.setMulticastTTL(0);
		ConnectResult res = sender.connect(group, 47777);
		CPPUNIT_ASSERT_EQUAL(ConnectResult::Connected, res);

		receiver.bind(47777);
		receiver.joinGroup(group);

		IOResult sent = sender.send(message, len);
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, sent.status);
		CPPUNIT_ASSERT_EQUAL(len, sent.bytes);

		char buffer[64] = {};
		IOResult recv = receiver.receive(buffer, len);
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, recv.status);
		CPPUNIT_ASSERT_EQUAL(len, recv.bytes);
		CPPUNIT_ASSERT(std::memcmp(message, buffer, len) == 0);

		receiver.leaveGroup(group);
		sent = sender.send(message, len);
		CPPUNIT_ASSERT(sent);

		recv = receiver.timedReceive(buffer, len, 50);
		CPPUNIT_ASSERT_EQUAL(IOStatus::Timeout, recv.status);
		CPPUNIT_ASSERT_EQUAL(0ul, recv.bytes);
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

	void testAdoptNativeHandle()
	{
		// A descriptor of the wrong socket type must be rejected without
		// stealing ownership from the caller.
		NET::UDPSocket systemdOwned;
		const int datagramFd = systemdOwned.duplicateNativeHandle();
		CPPUNIT_ASSERT(datagramFd >= 0);
		CPPUNIT_ASSERT_THROW(auto t = NET::TCPSocket::adoptNativeHandle(datagramFd), NET::SocketException);

		{
			NET::UDPSocket owner = NET::UDPSocket::adoptNativeHandle(datagramFd);
			CPPUNIT_ASSERT(owner.valid());
		}

		// The adopted descriptor is owned by the wrapper and must be closed.
		CPPUNIT_ASSERT_THROW(auto t = NET::UDPSocket::adoptNativeHandle(datagramFd),  NET::SocketException);
		CPPUNIT_ASSERT(systemdOwned.valid());
	}

	void testNonBlockingReceiveFrom()
	{
		NET::UDPSocket sender;
		NET::UDPSocket receiver;
		sender.bind(LOOPBACK, 0);
		receiver.bind(LOOPBACK, 0);
		receiver.setBlocking(false);

		char buffer[64];
		std::memset(buffer, 0x5a, sizeof(buffer));
		std::string address = "unchanged";
		unsigned short port = 54321;
		IOResult result = receiver.receiveFrom(buffer, sizeof(buffer), address, port);
		CPPUNIT_ASSERT_EQUAL(IOStatus::WouldBlock, result.status);
		CPPUNIT_ASSERT_EQUAL(size_t(0), result.bytes);
		CPPUNIT_ASSERT_EQUAL(std::string("unchanged"), address);
		CPPUNIT_ASSERT_EQUAL(static_cast<unsigned short>(54321), port);
		for(char c : buffer) CPPUNIT_ASSERT_EQUAL(char(0x5a), c);

		result = receiver.timedReceiveFrom(buffer, sizeof(buffer), address, port, 50);
		CPPUNIT_ASSERT_EQUAL(IOStatus::Timeout, result.status);
		CPPUNIT_ASSERT_EQUAL(size_t(0), result.bytes);
		CPPUNIT_ASSERT_EQUAL(std::string("unchanged"), address);
		CPPUNIT_ASSERT_EQUAL(static_cast<unsigned short>(54321), port);

		const char payload[] = "nonblocking UDP";
		IOResult sent = sender.sendTo(payload, sizeof(payload), LOOPBACK, receiver.getLocalPort());
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, sent.status);

		result = receiver.timedReceiveFrom(buffer, sizeof(buffer), address, port, 1000);
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, result.status);
		CPPUNIT_ASSERT_EQUAL(sizeof(payload), result.bytes);
		CPPUNIT_ASSERT_EQUAL(std::string(LOOPBACK), address);
		CPPUNIT_ASSERT_EQUAL(sender.getLocalPort(), port);
		CPPUNIT_ASSERT(std::memcmp(buffer, payload, sizeof(payload)) == 0);
	}

	void testNonBlockingZeroLengthDatagram()
	{
		NET::UDPSocket sender;
		NET::UDPSocket receiver;
		sender.bind(LOOPBACK, 0);
		receiver.bind(LOOPBACK, 0);
		receiver.setBlocking(false);

		char dummy = 'Q';
		IOResult sent = sender.sendTo(&dummy, 0, LOOPBACK, receiver.getLocalPort());
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, sent.status);
		CPPUNIT_ASSERT_EQUAL(size_t(0), sent.bytes);

		std::string address;
		unsigned short port = 0;
		IOResult result = receiver.timedReceiveFrom(&dummy, 1, address, port, 1000);
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, result.status);
		CPPUNIT_ASSERT_EQUAL(size_t(0), result.bytes);
		CPPUNIT_ASSERT_EQUAL(sender.getLocalPort(), port);
		CPPUNIT_ASSERT_EQUAL('Q', dummy);

		result = receiver.receiveFrom(&dummy, 1, address, port);
		CPPUNIT_ASSERT_EQUAL(IOStatus::WouldBlock, result.status);
		CPPUNIT_ASSERT_EQUAL(size_t(0), result.bytes);
	}

	void testNonBlockingConnectedReceive()
	{
		NET::UDPSocket sender;
		NET::UDPSocket receiver;
		receiver.bind(LOOPBACK, 0);

		ConnectResult res = sender.connect(LOOPBACK, receiver.getLocalPort());
		CPPUNIT_ASSERT_EQUAL(ConnectResult::Connected, res);
		receiver.setBlocking(false);

		char buffer[32]{};
		IOResult result = receiver.receive(buffer, sizeof(buffer));
		CPPUNIT_ASSERT_EQUAL(IOStatus::WouldBlock, result.status);

		const char payload[] = "UDP data";
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, sender.send(payload, sizeof(payload)).status);
		result = receiver.timedReceive(buffer, sizeof(buffer), 1000);
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, result.status);
		CPPUNIT_ASSERT_EQUAL(sizeof(payload), result.bytes);
		CPPUNIT_ASSERT(std::memcmp(buffer, payload, sizeof(payload)) == 0);
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION(UDPSocket_TEST);
