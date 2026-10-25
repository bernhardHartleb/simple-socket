#include <cppunit/extensions/HelperMacros.h>
#include "../TCPSocket.h"

#include <cstring>
#include <vector>

class TCPSocket_TEST : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(TCPSocket_TEST);
	CPPUNIT_TEST(testConnectAcceptAndTransfer);
	CPPUNIT_TEST(testSendAll);
	CPPUNIT_TEST(testTimedAccept);
	CPPUNIT_TEST(testTimedReceive);
	CPPUNIT_TEST(testTimedConnect);
	CPPUNIT_TEST(testGracefulShutdown);
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

		client.connect(LOOPBACK, port);

		NET::TCPSocket::Handle handle = server.accept();
		CPPUNIT_ASSERT(handle);

		NET::TCPSocket session(std::move(handle));
		CPPUNIT_ASSERT(!handle);
		CPPUNIT_ASSERT(!client.peerDisconnected());
		CPPUNIT_ASSERT(!session.peerDisconnected());

		CPPUNIT_ASSERT_THROW(NET::TCPSocket(std::move(handle)), NET::SocketException);
		CPPUNIT_ASSERT_EQUAL(std::string(LOOPBACK), client.getForeignAddress());
		CPPUNIT_ASSERT_EQUAL(std::string(LOOPBACK), session.getLocalAddress());
		CPPUNIT_ASSERT_EQUAL(port, client.getForeignPort());
		CPPUNIT_ASSERT_EQUAL(port, session.getLocalPort());

		const char message[] = "Hello TCP";
		const int sent = client.send(message, sizeof(message));

		CPPUNIT_ASSERT(sent > 0);
		CPPUNIT_ASSERT(sent <= static_cast<int>(sizeof(message)));

		char buffer[sizeof(message)] = {};
		const int received = session.receive(buffer, static_cast<size_t>(sent));

		CPPUNIT_ASSERT_EQUAL(sent, received);
		CPPUNIT_ASSERT(std::memcmp(message, buffer, static_cast<size_t>(sent)) == 0);
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
		for (size_t i = 0; i < sendBuffer.size(); ++i)
			sendBuffer[i] = static_cast<char>(i % 251);

		const int sent = client.sendAll(sendBuffer.data(), sendBuffer.size());
		CPPUNIT_ASSERT_EQUAL(static_cast<int>(sendBuffer.size()), sent);

		std::vector<char> receiveBuffer(sendBuffer.size());
		size_t received = 0;

		while (received < receiveBuffer.size())
		{
			const int ret = session.receive(
				receiveBuffer.data() + received,
				receiveBuffer.size() - received);

			CPPUNIT_ASSERT(ret > 0);
			received += static_cast<size_t>(ret);
		}

		CPPUNIT_ASSERT_EQUAL(sendBuffer.size(), received);
		CPPUNIT_ASSERT(sendBuffer == receiveBuffer);
	}

	void testTimedAccept()
	{
		NET::TCPSocket server;
		NET::TCPSocket client;

		const unsigned short port = prepareServer(server);

		// No client yet, this should simply time out
		NET::TCPSocket::Handle handle = server.timedAccept(50);
		CPPUNIT_ASSERT(!handle);

		client.connect(LOOPBACK, port);

		handle = server.timedAccept(1000);
		CPPUNIT_ASSERT(handle);

		NET::TCPSocket session(std::move(handle));
		CPPUNIT_ASSERT(!handle);
		CPPUNIT_ASSERT(!session.peerDisconnected());
	}

	void testTimedReceive()
	{
		NET::TCPSocket server;
		NET::TCPSocket client;

		const unsigned short port = prepareServer(server);
		client.connect(LOOPBACK, port);

		NET::TCPSocket session(server.accept());

		char buffer[64] = {};
		// Connected socket, but no data waiting
		int ret = session.timedReceive(buffer, sizeof(buffer), 50);
		CPPUNIT_ASSERT_EQUAL(0, ret);
		// Timeout must not be mistaken for peer disconnect
		CPPUNIT_ASSERT(!session.peerDisconnected());

		const char message[] = "Timed receive";
		const int sent = client.sendAll(message, sizeof(message));
		CPPUNIT_ASSERT_EQUAL(static_cast<int>(sizeof(message)), sent);

		ret = session.timedReceive(buffer, sizeof(buffer), 1000);
		CPPUNIT_ASSERT(ret > 0);
		CPPUNIT_ASSERT(ret <= static_cast<int>(sizeof(message)));
		CPPUNIT_ASSERT(std::memcmp(message, buffer, static_cast<size_t>(ret)) == 0);
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
		// reset connection attempt
		client.disconnect();
		CPPUNIT_ASSERT(!client.peerDisconnected());

		const unsigned short port = prepareServer(server);
		const int ret = client.timedConnect(LOOPBACK, port, 1000);

		CPPUNIT_ASSERT_EQUAL(1, ret);
		CPPUNIT_ASSERT_EQUAL(std::string(LOOPBACK), client.getForeignAddress());
		CPPUNIT_ASSERT_EQUAL(port, client.getForeignPort());

		NET::TCPSocket::Handle handle = server.accept();
		CPPUNIT_ASSERT(handle);

		NET::TCPSocket session(std::move(handle));
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
		client.shutdown(NET::SimpleSocket::STOP_SEND);

		char buffer[16];
		const int ret = session.timedReceive(buffer, sizeof(buffer), 1000);
		CPPUNIT_ASSERT_EQUAL(0, ret);
		CPPUNIT_ASSERT(session.peerDisconnected());
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION(TCPSocket_TEST);
