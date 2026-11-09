#include <cppunit/extensions/HelperMacros.h>
#include <simple_socket/TCPSocket.h>

#include <cstring>
#include <vector>
#include <chrono>

using NET::IOResult;
using NET::IOStatus;
using NET::ConnectResult;

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
	CPPUNIT_TEST(testAdoptNativeHandle);
	CPPUNIT_TEST(testNonBlockingAccept);
	CPPUNIT_TEST(testNonBlockingConnect);
	CPPUNIT_TEST(testNonBlockingReceiveAndTimeout);
	CPPUNIT_TEST(testNonBlockingSendWouldBlock);
	CPPUNIT_TEST(testNonBlockingAggregateProgress);
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

		ConnectResult res = client.connect(LOOPBACK, port);
		CPPUNIT_ASSERT_EQUAL(ConnectResult::Connected, res);

		NET::TCPSocket session = *server.accept();
		CPPUNIT_ASSERT(!client.peerDisconnected());
		CPPUNIT_ASSERT(!session.peerDisconnected());

		CPPUNIT_ASSERT_EQUAL(std::string(LOOPBACK), client.getForeignAddress());
		CPPUNIT_ASSERT_EQUAL(std::string(LOOPBACK), session.getLocalAddress());
		CPPUNIT_ASSERT_EQUAL(port, client.getForeignPort());
		CPPUNIT_ASSERT_EQUAL(port, session.getLocalPort());

		const char message[] = "Hello TCP";
		IOResult sent = client.send(message, sizeof(message));
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, sent.status);
		CPPUNIT_ASSERT_EQUAL(sizeof(message), sent.bytes);

		char buffer[sizeof(message)] = {};
		IOResult recv = session.receive(buffer, sent.bytes);
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, recv.status);
		CPPUNIT_ASSERT_EQUAL(recv.bytes, sent.bytes);
		CPPUNIT_ASSERT(std::memcmp(message, buffer, sent.bytes) == 0);
	}

	void testSendAll()
	{
		NET::TCPSocket server;
		NET::TCPSocket client;

		const unsigned short port = prepareServer(server);
		ConnectResult res = client.connect(LOOPBACK, port);
		CPPUNIT_ASSERT_EQUAL(ConnectResult::Connected, res);

		NET::TCPSocket session(*server.accept());

		// Large enough to exercise more than just a trivial message
		std::vector<char> sendBuffer(32 * 1024);
		const size_t len = sendBuffer.size();

		for (size_t i = 0; i < sendBuffer.size(); ++i)
			sendBuffer[i] = static_cast<char>(i % 251);

		IOResult sent = client.sendAll(sendBuffer.data(), sendBuffer.size());
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, sent.status);
		CPPUNIT_ASSERT_EQUAL(len, sent.bytes);

		std::vector<char> receiveBuffer(sendBuffer.size());
		IOResult recv = session.receiveAll(receiveBuffer.data(), receiveBuffer.size());
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, recv.status);
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

		ConnectResult res = client.connect(LOOPBACK, port);
		CPPUNIT_ASSERT_EQUAL(ConnectResult::Connected, res);

		opt = server.timedAccept(1000);
		CPPUNIT_ASSERT(opt);
		CPPUNIT_ASSERT(opt->valid());

		// non-blocking connect attempt return right away
		auto tmp = server.timedAccept(0);
		CPPUNIT_ASSERT(!tmp);

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
		ConnectResult res = client.connect(LOOPBACK, port);
		CPPUNIT_ASSERT_EQUAL(ConnectResult::Connected, res);
		// Does not influce results below
		client.setNoDelay(true);

		NET::TCPSocket session(*server.accept());

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

		ConnectResult err = client.timedConnect(LOOPBACK, refusedPort, 50);
		CPPUNIT_ASSERT_EQUAL(ConnectResult::Refused, err);
		// reset connection attempt (this works for TCP only on Linux)
		client.disconnect();
		CPPUNIT_ASSERT(client.peerDisconnected());

		const unsigned short port = prepareServer(server);
		ConnectResult res = client.timedConnect(LOOPBACK, port, 1000);

		CPPUNIT_ASSERT_EQUAL(ConnectResult::Connected, res);
		CPPUNIT_ASSERT_EQUAL(std::string(LOOPBACK), client.getForeignAddress());
		CPPUNIT_ASSERT_EQUAL(port, client.getForeignPort());

		NET::TCPSocket session = *server.accept();;
		CPPUNIT_ASSERT(!client.peerDisconnected());
		CPPUNIT_ASSERT(!session.peerDisconnected());
	}

	void testGracefulShutdown()
	{
		NET::TCPSocket server;
		NET::TCPSocket client;
		const auto port = prepareServer(server);

		// finishConnect must not mistake an untouched socket for a connection
		CPPUNIT_ASSERT_THROW(auto t = client.finishConnect(0), NET::SocketException);
		CPPUNIT_ASSERT(!client.isConnected());

		ConnectResult res = client.connect(LOOPBACK, port);
		CPPUNIT_ASSERT_EQUAL(ConnectResult::Connected, res);
		CPPUNIT_ASSERT(client.isConnected());

		NET::TCPSocket session(*server.accept());
		CPPUNIT_ASSERT(client.isConnected());
		CPPUNIT_ASSERT(session.isConnected());
		CPPUNIT_ASSERT(!session.peerDisconnected());

		// Send a FIN while retaining the client socket itself
		client.shutdown(NET::STOP_SEND);

		char buffer[16];
		IOResult ret = session.timedReceive(buffer, sizeof(buffer), 1000);
		CPPUNIT_ASSERT_EQUAL(IOStatus::PeerDisconnected, ret.status);
		CPPUNIT_ASSERT_EQUAL(0ul, ret.bytes);

		// A peer that has sent FIN can still have an associated address
		CPPUNIT_ASSERT(session.peerDisconnected());
		CPPUNIT_ASSERT(session.isConnected());

		session.disconnect(); // this works for TCP only on Linux
		CPPUNIT_ASSERT(session.peerDisconnected());
		CPPUNIT_ASSERT(!session.isConnected());
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
		CPPUNIT_ASSERT_THROW(auto t = client.connect(LOOPBACK, port), NET::SocketException);

		ConnectResult res = client.timedConnect("::1", port, 1000);
		CPPUNIT_ASSERT_EQUAL(ConnectResult::Connected, res);
		CPPUNIT_ASSERT_EQUAL(NET::IPv6, client.addressFamily());

		NET::TCPSocket session(*server.accept());
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

	void testAdoptNativeHandle()
	{
		NET::TCPSocket systemdOwned;
		const unsigned short port = prepareServer(systemdOwned);

		// systemd and the service each hold a descriptor referring to the same
		// listening socket. dup() models the descriptor handed to the service.
		const int passedFd = systemdOwned.duplicateNativeHandle();
		CPPUNIT_ASSERT(passedFd >= 0);

		{
			NET::TCPSocket listener = NET::TCPSocket::adoptNativeHandle(passedFd);
			CPPUNIT_ASSERT(listener.valid());
			CPPUNIT_ASSERT_EQUAL(passedFd, listener.nativeHandle());
			CPPUNIT_ASSERT_EQUAL(NET::IPv4, listener.addressFamily());
			CPPUNIT_ASSERT_EQUAL(port, listener.getLocalPort());

			NET::TCPSocket client;
			ConnectResult res = client.connect(LOOPBACK, port);
			CPPUNIT_ASSERT_EQUAL(ConnectResult::Connected, res);
			NET::TCPSocket session = *listener.accept();
			CPPUNIT_ASSERT(!session.peerDisconnected());
		}

		// The adopted descriptor is owned by the wrapper and must be closed.
		CPPUNIT_ASSERT_THROW(auto t = NET::TCPSocket::adoptNativeHandle(passedFd), NET::SocketException);
		CPPUNIT_ASSERT(systemdOwned.valid());
	}

	void testNonBlockingAccept()
	{
		NET::TCPSocket listener;
		const unsigned short port = prepareServer(listener);
		listener.setBlocking(false);
		CPPUNIT_ASSERT(!listener.isBlocking());
		CPPUNIT_ASSERT(!listener.accept());
		CPPUNIT_ASSERT(!listener.timedAccept(0));

		NET::TCPSocket client;
		ConnectResult res = client.connect(LOOPBACK, port);
		CPPUNIT_ASSERT_EQUAL(ConnectResult::Connected, res);
		auto accepted = listener.timedAccept(1000);
		CPPUNIT_ASSERT(accepted.has_value());
		// non-blocking mode is inherited
		CPPUNIT_ASSERT(!accepted->isBlocking());
	}

	void testNonBlockingConnect()
	{
		NET::TCPSocket listener;
		const unsigned short port = prepareServer(listener);
		NET::TCPSocket client;
		client.setBlocking(false);

		ConnectResult result = client.connect(LOOPBACK, port);
		if(result == ConnectResult::InProgress) { result = client.finishConnect(1000); }
		CPPUNIT_ASSERT_EQUAL(ConnectResult::Connected, result);

		auto accepted = listener.timedAccept(1000);
		CPPUNIT_ASSERT(accepted.has_value());

		const char payload[] = "connected";
		const IOResult sent = client.send(payload, sizeof(payload));
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, sent.status);
		CPPUNIT_ASSERT_EQUAL(sizeof(payload), sent.bytes);

		char buffer[sizeof(payload)]{};
		const IOResult received = accepted->timedReceive(buffer, sizeof(buffer), 1000);
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, received.status);
		CPPUNIT_ASSERT_EQUAL(sizeof(payload), received.bytes);
		CPPUNIT_ASSERT(std::memcmp(payload, buffer, sizeof(payload)) == 0);
	}

	void testNonBlockingReceiveAndTimeout()
	{
		NET::TCPSocket listener;
		const unsigned short port = prepareServer(listener);
		NET::TCPSocket client;
		ConnectResult res = client.connect(LOOPBACK, port);
		CPPUNIT_ASSERT_EQUAL(ConnectResult::Connected, res);

		NET::TCPSocket session(*listener.accept());
		session.setBlocking(false);

		char buffer[64];
		std::memset(buffer, 0x5a, sizeof(buffer));
		IOResult result = session.receive(buffer, sizeof(buffer));
		CPPUNIT_ASSERT_EQUAL(IOStatus::WouldBlock, result.status);
		CPPUNIT_ASSERT_EQUAL(size_t(0), result.bytes);
		for(char c : buffer) CPPUNIT_ASSERT_EQUAL(char(0x5a), c);

		result = session.timedReceive(buffer, sizeof(buffer), 0);
		CPPUNIT_ASSERT_EQUAL(IOStatus::Timeout, result.status);

		const auto start = std::chrono::steady_clock::now();
		result = session.timedReceive(buffer, sizeof(buffer), 50);
		const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
		    std::chrono::steady_clock::now() - start).count();
		CPPUNIT_ASSERT_EQUAL(IOStatus::Timeout, result.status);
		CPPUNIT_ASSERT_EQUAL(size_t(0), result.bytes);
		CPPUNIT_ASSERT(elapsed >= 20); // must not immediately return WouldBlock
		CPPUNIT_ASSERT(elapsed < 2000); // generous CI scheduling margin

		const char payload[] = "ready";
		result = client.sendAll(payload, sizeof(payload));
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, result.status);

		result = session.timedReceive(buffer, sizeof(buffer), 1000);
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, result.status);
		CPPUNIT_ASSERT_EQUAL(sizeof(payload), result.bytes);
		CPPUNIT_ASSERT(std::memcmp(buffer, payload, sizeof(payload)) == 0);

		result = session.receive(buffer, sizeof(buffer));
		CPPUNIT_ASSERT_EQUAL(IOStatus::WouldBlock, result.status);
	}

	void testNonBlockingSendWouldBlock()
	{
		NET::TCPSocket listener;
		const unsigned short port = prepareServer(listener);
		NET::TCPSocket client;
		ConnectResult res = client.connect(LOOPBACK, port);
		CPPUNIT_ASSERT_EQUAL(ConnectResult::Connected, res);

		NET::TCPSocket session(*listener.accept());
		client.setSendBufferSize(4096);
		client.setBlocking(false);

		std::vector<char> chunk(64 * 1024, 'x');
		bool sawWouldBlock = false;
		size_t progress = 0;
		for(unsigned i = 0; i < 4096; ++i) // no infinite loop
		{
			const IOResult result = client.send(chunk.data(), chunk.size());
			if(result.status == IOStatus::WouldBlock)
			{
				CPPUNIT_ASSERT_EQUAL(size_t(0), result.bytes);
				sawWouldBlock = true;
				break;
			}
			CPPUNIT_ASSERT_EQUAL(IOStatus::Success, result.status);
			CPPUNIT_ASSERT(result.bytes > 0);
			CPPUNIT_ASSERT(result.bytes <= chunk.size());
			progress += result.bytes;
		}
		CPPUNIT_ASSERT(progress > 0);
		CPPUNIT_ASSERT(sawWouldBlock);
	}

	void testNonBlockingAggregateProgress()
	{
		// receiveAll() reports partial progress when the stream runs dry.
		{
			NET::TCPSocket listener;
			const unsigned short port = prepareServer(listener);
			NET::TCPSocket client;
			ConnectResult res = client.connect(LOOPBACK, port);
			CPPUNIT_ASSERT_EQUAL(ConnectResult::Connected, res);

			NET::TCPSocket session(*listener.accept());
			session.setBlocking(false);
			const char first[] = "first";
			const char second[] = "second";
			CPPUNIT_ASSERT_EQUAL(IOStatus::Success, client.sendAll(first, 5).status);
			char buffer[11]{};
			IOResult result = session.receiveAll(buffer, sizeof(buffer));
			CPPUNIT_ASSERT_EQUAL(IOStatus::WouldBlock, result.status);
			CPPUNIT_ASSERT_EQUAL(size_t(5), result.bytes);
			CPPUNIT_ASSERT(std::memcmp(buffer, first, 5) == 0);
			CPPUNIT_ASSERT_EQUAL(IOStatus::Success, client.sendAll(second, 6).status);

			result = session.receiveAll(buffer + 5, 6);
			CPPUNIT_ASSERT_EQUAL(IOStatus::Success, result.status);
			CPPUNIT_ASSERT_EQUAL(size_t(6), result.bytes);
			CPPUNIT_ASSERT(std::memcmp(buffer, "firstsecond", sizeof(buffer)) == 0);
		}
		// sendAll() must return WouldBlock plus accepted bytes, not silently lose progress.
		{
			NET::TCPSocket listener;
			const unsigned short port = prepareServer(listener);
			NET::TCPSocket client;
			ConnectResult res = client.connect(LOOPBACK, port);
			CPPUNIT_ASSERT_EQUAL(ConnectResult::Connected, res);

			NET::TCPSocket session(*listener.accept());
			(void)session;
			client.setSendBufferSize(4096);
			client.setBlocking(false);
			std::vector<char> payload(8 * 1024 * 1024, 'z');
			const IOResult result = client.sendAll(payload.data(), payload.size());
			CPPUNIT_ASSERT_EQUAL(IOStatus::WouldBlock, result.status);
			CPPUNIT_ASSERT(result.bytes > 0);
			CPPUNIT_ASSERT(result.bytes < payload.size());
		}
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION(TCPSocket_TEST);
