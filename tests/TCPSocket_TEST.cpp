#include <cppunit/extensions/HelperMacros.h>
#include <simple_socket/TCPSocket.h>

#include <cstring>
#include <vector>

using NET::IOResult;
using NET::IOStatus;

class TCPSocket_TEST : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(TCPSocket_TEST);
	CPPUNIT_TEST(testConnectAcceptAndTransfer);
	CPPUNIT_TEST(testSendAll);
	CPPUNIT_TEST(testTimedAccept);
	CPPUNIT_TEST(testTimedReceive);
	CPPUNIT_TEST(testTimedConnect);
	CPPUNIT_TEST(testGracefulShutdown);
	CPPUNIT_TEST(testIPv6);
	CPPUNIT_TEST_SUITE_END();

private:
	static constexpr const char* LOOPBACK = "127.0.0.1";

	static unsigned short prepareServer(NET::TCPSocket& server)
	{
		// Let the OS choose a free port
		server.bind(LOOPBACK, 0);
		server.listen();
		return server.getLocalPort();
	}

public:
	void testConnectAcceptAndTransfer()
	{
		NET::TCPSocket server;
		NET::TCPSocket client;

		const unsigned short port = prepareServer(server);
		CPPUNIT_ASSERT(port != 0);

		// No peer exists before connect()
		CPPUNIT_ASSERT_EQUAL(std::string(), client.getForeignAddress());
		CPPUNIT_ASSERT_EQUAL(static_cast<unsigned short>(0), client.getForeignPort());

		client.connect(LOOPBACK, port);

		NET::TCPSocket session = server.accept();
		CPPUNIT_ASSERT(!client.peerDisconnected());
		CPPUNIT_ASSERT(!session.peerDisconnected());

		CPPUNIT_ASSERT_EQUAL(std::string(LOOPBACK), client.getForeignAddress());
		CPPUNIT_ASSERT_EQUAL(std::string(LOOPBACK), session.getLocalAddress());
		CPPUNIT_ASSERT_EQUAL(port, client.getForeignPort());
		CPPUNIT_ASSERT_EQUAL(port, session.getLocalPort());

		const char message[] = "Hello TCP";
		IOResult sent = client.send(message, sizeof(message));
		CPPUNIT_ASSERT_EQUAL(sent.status, IOStatus::Success);
		CPPUNIT_ASSERT_EQUAL(sent.bytes, sizeof(message));

		char buffer[sizeof(message)] = {};
		IOResult recv = session.receive(buffer, sent.bytes);
		CPPUNIT_ASSERT_EQUAL(recv.status, IOStatus::Success);
		CPPUNIT_ASSERT_EQUAL(recv.bytes, sent.bytes);
		CPPUNIT_ASSERT(std::memcmp(message, buffer, sent.bytes) == 0);
	}

	void testSendAll()
	{
		NET::TCPSocket server;
		NET::TCPSocket client;

		const unsigned short port = prepareServer(server);
		client.connect(LOOPBACK, port);

		NET::TCPSocket session(server.accept());

		// Large enough to exercise more than just a trivial message
		std::vector<char> sendBuffer(32 * 1024);
		const size_t len = sendBuffer.size();

		for (size_t i = 0; i < sendBuffer.size(); ++i)
			sendBuffer[i] = static_cast<char>(i % 251);

		IOResult sent = client.sendAll(sendBuffer.data(), sendBuffer.size());
		CPPUNIT_ASSERT_EQUAL(sent.status, IOStatus::Success);
		CPPUNIT_ASSERT_EQUAL(len, sent.bytes);

		std::vector<char> receiveBuffer(sendBuffer.size());
		IOResult recv = session.receiveAll(receiveBuffer.data(), receiveBuffer.size());
		CPPUNIT_ASSERT_EQUAL(recv.status, IOStatus::Success);
		CPPUNIT_ASSERT_EQUAL(len, recv.bytes);
		CPPUNIT_ASSERT(sendBuffer == receiveBuffer);
	}

	void testTimedAccept()
	{
		NET::TCPSocket server;
		NET::TCPSocket client;

		const unsigned short port = prepareServer(server);

		// No client yet, this should simply time out
		auto opt = server.timedAccept(50);
		CPPUNIT_ASSERT(!opt);

		client.connect(LOOPBACK, port);

		opt = server.timedAccept(1000);
		CPPUNIT_ASSERT(opt);
		CPPUNIT_ASSERT(opt->valid());

		NET::TCPSocket session(std::move(*opt));
		CPPUNIT_ASSERT(!opt->valid());
		opt.reset();
		CPPUNIT_ASSERT(!opt);
		CPPUNIT_ASSERT(!session.peerDisconnected());

		// try send after shutdown
		session.shutdown(NET::STOP_BOTH);
		char dummy = 0;
		IOResult sent = session.send( &dummy, sizeof(dummy));
		CPPUNIT_ASSERT_EQUAL(IOStatus::PeerDisconnected, sent.status);
		CPPUNIT_ASSERT_EQUAL(0ul, sent.bytes);
	}

	void testTimedReceive()
	{
		NET::TCPSocket server;
		NET::TCPSocket client;

		const unsigned short port = prepareServer(server);
		client.connect(LOOPBACK, port);
		// Does not influce results below
		client.setNoDelay(true);

		NET::TCPSocket session(server.accept());

		char buffer[64] = {};
		// Connected socket, but no data waiting
		IOResult ret = session.timedReceive(buffer, sizeof(buffer), 50);
		CPPUNIT_ASSERT_EQUAL(IOStatus::Timeout, ret.status);
		CPPUNIT_ASSERT_EQUAL(0ul, ret.bytes);
		// Timeout must not be mistaken for peer disconnect
		CPPUNIT_ASSERT(!session.peerDisconnected());

		const char message[] = "Timed receive";
		IOResult sent = client.sendAll(message, sizeof(message));
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, sent.status);
		CPPUNIT_ASSERT_EQUAL(sizeof(message), sent.bytes);

		ret = session.timedReceive(buffer, sizeof(buffer), 1000);
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, ret.status);
		CPPUNIT_ASSERT(ret.bytes == sizeof(message));
		CPPUNIT_ASSERT(std::memcmp(message, buffer, ret.bytes) == 0);
		CPPUNIT_ASSERT(!session.peerDisconnected());
	}

	void testTimedConnect()
	{
		NET::TCPSocket server;
		NET::TCPSocket client;

		// Reserve a local TCP port, but deliberately do not listen on it
		// This gives us a deterministic connection-refused endpoint
		NET::TCPSocket notListening;
		notListening.bind(LOOPBACK, 0);
		const unsigned short refusedPort = notListening.getLocalPort();
		CPPUNIT_ASSERT(refusedPort != 0);

		CPPUNIT_ASSERT_THROW(client.timedConnect(LOOPBACK, refusedPort, 1000), NET::SocketException);
		// reset connection attempt (this works for TCP only on Linux)
		client.disconnect();
		CPPUNIT_ASSERT(client.peerDisconnected());

		const unsigned short port = prepareServer(server);
		const int ret = client.timedConnect(LOOPBACK, port, 1000);

		CPPUNIT_ASSERT_EQUAL(1, ret);
		CPPUNIT_ASSERT_EQUAL(std::string(LOOPBACK), client.getForeignAddress());
		CPPUNIT_ASSERT_EQUAL(port, client.getForeignPort());

		NET::TCPSocket session = server.accept();;
		CPPUNIT_ASSERT(!client.peerDisconnected());
		CPPUNIT_ASSERT(!session.peerDisconnected());
	}

	void testGracefulShutdown()
	{
		NET::TCPSocket server;
		NET::TCPSocket client;

		const unsigned short port = prepareServer(server);
		client.connect(LOOPBACK, port);

		NET::TCPSocket session(server.accept());
		CPPUNIT_ASSERT(!session.peerDisconnected());

		// Send a FIN while retaining the client socket itself
		client.shutdown(NET::STOP_SEND);

		char buffer[16];
		IOResult ret = session.timedReceive(buffer, sizeof(buffer), 1000);
		CPPUNIT_ASSERT_EQUAL(IOStatus::PeerDisconnected, ret.status);
		CPPUNIT_ASSERT_EQUAL(0ul, ret.bytes);
		CPPUNIT_ASSERT(session.peerDisconnected());

		session.disconnect(); // this works for TCP only on Linux
		CPPUNIT_ASSERT(session.peerDisconnected());
	}

	void testIPv6()
	{
		NET::TCPSocket server(NET::IPv6);
		// ipv4 should not work
		CPPUNIT_ASSERT_THROW(server.bind(LOOPBACK), NET::SocketException);

		server.bind("::1", 0);
		server.listen();
		const unsigned short port = server.getLocalPort();

		NET::TCPSocket client(NET::IPv6);
		// ipv4 should not work
		CPPUNIT_ASSERT_THROW(client.connect(LOOPBACK, port), NET::SocketException);

		const int ret = client.timedConnect("::1", port, 1000);
		CPPUNIT_ASSERT_EQUAL(1, ret);
		CPPUNIT_ASSERT_EQUAL(NET::IPv6, client.addressFamily());

		NET::TCPSocket session(server.accept());
		CPPUNIT_ASSERT_EQUAL(std::string("::1"), client.getForeignAddress());
		CPPUNIT_ASSERT_EQUAL(std::string("::1"), session.getForeignAddress());
		CPPUNIT_ASSERT_EQUAL(NET::IPv6, session.addressFamily());

		const char message[] = "IPv6 works";
		IOResult sent = client.sendAll(message, sizeof(message));
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, sent.status);
		CPPUNIT_ASSERT_EQUAL(sizeof(message), sent.bytes);

		char buffer[sizeof(message)]{};
		IOResult recv = session.timedReceive(buffer, sizeof(buffer), 1000);
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, recv.status);
		CPPUNIT_ASSERT_EQUAL(sizeof(message), recv.bytes);
		CPPUNIT_ASSERT(std::memcmp(message, buffer, sizeof(message)) == 0);
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION(TCPSocket_TEST);
