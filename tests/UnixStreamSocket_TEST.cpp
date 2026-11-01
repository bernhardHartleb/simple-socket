#include <cppunit/extensions/HelperMacros.h>
#include <simple_socket/UnixStreamSocket.h>

#include <unistd.h>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

using NET::IOResult;
using NET::IOStatus;

class UnixStreamSocket_TEST : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(UnixStreamSocket_TEST);
	CPPUNIT_TEST(testConnectAcceptAndTransfer);
	CPPUNIT_TEST(testSendAll);
	CPPUNIT_TEST(testTimedAccept);
	CPPUNIT_TEST(testTimedReceive);
	CPPUNIT_TEST(testGracefulShutdown);
	CPPUNIT_TEST(testSocketPathOwnership);
	CPPUNIT_TEST_SUITE_END();

private:
	static constexpr const char* SERVER_SOCK = "unix-stream-server";
	static constexpr const char* CLIENT_SOCK = "unix-stream-client";

	static std::string socketPath(const char* name)
	{
		return std::string("/tmp/simple-socket-")
		     + std::to_string(static_cast<long long>(::getpid()))
		     + "-" + name + ".sock";
	}

	static void removeStaleSocket(const std::string& path)
	{
		::unlink(path.c_str());
	}

public:
	void testConnectAcceptAndTransfer()
	{
		const std::string serverPath = socketPath(SERVER_SOCK);
		const std::string clientPath = socketPath(CLIENT_SOCK);

		removeStaleSocket(serverPath);
		removeStaleSocket(clientPath);

		NET::UnixStreamSocket server;
		NET::UnixStreamSocket client;

		// No peer exists before connect()
		CPPUNIT_ASSERT_EQUAL(std::string(), client.getForeignPath());

		server.bind(serverPath);
		server.listen();

		client.bind(clientPath);
		client.connect(serverPath);

		NET::UnixStreamSocket session = server.accept();
		CPPUNIT_ASSERT_EQUAL(serverPath, server.getLocalPath());
		CPPUNIT_ASSERT_EQUAL(clientPath, client.getLocalPath());
		CPPUNIT_ASSERT_EQUAL(serverPath, client.getForeignPath());
		CPPUNIT_ASSERT_EQUAL(serverPath, session.getLocalPath());
		CPPUNIT_ASSERT_EQUAL(clientPath, session.getForeignPath());

		const char message[] = "Hello Unix stream";
		size_t sent = client.sendAll(message, sizeof(message));
		//CPPUNIT_ASSERT_EQUAL(IOStatus::Success, sent.status);
		CPPUNIT_ASSERT_EQUAL(sizeof(message), sent);

		char buffer[sizeof(message)] = {};
		IOResult recv = session.receive(buffer, sizeof(message));
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, recv.status);
		CPPUNIT_ASSERT_EQUAL(sizeof(message), recv.bytes);
		CPPUNIT_ASSERT(std::memcmp(message, buffer, sizeof(message)) == 0);
	}

	void testSendAll()
	{
		const std::string serverPath = socketPath(SERVER_SOCK);
		removeStaleSocket(serverPath);

		NET::UnixStreamSocket server;
		NET::UnixStreamSocket client;

		server.bind(serverPath);
		server.listen();
		client.connect(serverPath);

		NET::UnixStreamSocket session(server.accept());

		std::vector<char> sendBuffer(32 * 1024);
		const size_t len = sendBuffer.size();

		for (size_t i = 0; i < sendBuffer.size(); ++i)
			sendBuffer[i] = static_cast<char>(i % 251);

		size_t sent = client.sendAll(sendBuffer.data(), sendBuffer.size());
		CPPUNIT_ASSERT_EQUAL(len, sent);

		std::vector<char> receiveBuffer(sendBuffer.size());
		size_t received = session.receiveAll(receiveBuffer.data(), receiveBuffer.size());
		CPPUNIT_ASSERT_EQUAL(len, received);
		CPPUNIT_ASSERT(sendBuffer == receiveBuffer);
	}

	void testTimedAccept()
	{
		const std::string serverPath = socketPath(SERVER_SOCK);
		removeStaleSocket(serverPath);

		NET::UnixStreamSocket server;

		server.bind(serverPath);
		server.listen();

		// Nobody is connecting yet
		auto opt = server.timedAccept(50);
		CPPUNIT_ASSERT(!opt);

		NET::UnixStreamSocket client;
		client.connect(serverPath);

		opt = server.timedAccept(1000);
		CPPUNIT_ASSERT(opt);
		CPPUNIT_ASSERT(opt->valid());

		NET::UnixStreamSocket session(std::move(*opt));
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
		const std::string serverPath = socketPath(SERVER_SOCK);
		removeStaleSocket(serverPath);

		NET::UnixStreamSocket server;
		NET::UnixStreamSocket client;

		server.bind(serverPath);
		server.listen();
		client.connect(serverPath);

		NET::UnixStreamSocket session(server.accept());

		char buffer[64] = {};
		// Connection exists, but no data is waiting
		IOResult recv = session.timedReceive(buffer, sizeof(buffer), 50);
		CPPUNIT_ASSERT_EQUAL(IOStatus::Timeout, recv.status);
		CPPUNIT_ASSERT_EQUAL(0ul, recv.bytes);
		// Timeout must not be mistaken for peer disconnect
		CPPUNIT_ASSERT(!session.peerDisconnected());

		const char message[] = "Timed Unix stream";
		size_t sent = client.sendAll(message, sizeof(message));
		//CPPUNIT_ASSERT_EQUAL(IOStatus::Success, sent.status);
		CPPUNIT_ASSERT_EQUAL(sizeof(message), sent);

		recv = session.timedReceive(buffer, sizeof(buffer), 1000);
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, recv.status);
		CPPUNIT_ASSERT_EQUAL(sizeof(message), recv.bytes);
		CPPUNIT_ASSERT(std::memcmp(message,buffer, recv.bytes) == 0);
		CPPUNIT_ASSERT(!session.peerDisconnected());
	}

	void testGracefulShutdown()
	{
		const std::string serverPath = socketPath("unix-stream-shutdown");
		removeStaleSocket(serverPath);

		NET::UnixStreamSocket server;
		NET::UnixStreamSocket client;

		server.bind(serverPath);
		server.listen();
		client.connect(serverPath);

		NET::UnixStreamSocket session(server.accept());
		CPPUNIT_ASSERT(!session.peerDisconnected());

		client.shutdown(NET::STOP_SEND);

		char buffer[16];
		IOResult recv = session.timedReceive(buffer, sizeof(buffer), 1000);
		CPPUNIT_ASSERT_EQUAL(IOStatus::PeerDisconnected, recv.status);
		CPPUNIT_ASSERT_EQUAL(0ul, recv.bytes);
		// Distinguishes orderly stream EOF from a normal timeout
		CPPUNIT_ASSERT(session.peerDisconnected());
	}

	void testSocketPathOwnership()
	{
		const std::string serverPath = socketPath(SERVER_SOCK);
		const std::string clientPath = socketPath(CLIENT_SOCK);

		removeStaleSocket(serverPath);
		removeStaleSocket(clientPath);

		CPPUNIT_ASSERT(::access(serverPath.c_str(), F_OK) != 0);
		CPPUNIT_ASSERT(::access(clientPath.c_str(), F_OK) != 0);

		{
			NET::UnixStreamSocket server;
			server.bind(serverPath);
			server.listen();

			CPPUNIT_ASSERT(::access(serverPath.c_str(), F_OK) == 0);

			{
				NET::UnixStreamSocket client;
				client.bind(clientPath);
				client.connect(serverPath);

				CPPUNIT_ASSERT(::access(clientPath.c_str(), F_OK) == 0);

				{
					NET::UnixStreamSocket session(server.accept());
					CPPUNIT_ASSERT_EQUAL(serverPath, session.getLocalPath());
					CPPUNIT_ASSERT_EQUAL(clientPath, session.getForeignPath());
				}

				// Destroying an accepted connection must not unlink the listening socket
				CPPUNIT_ASSERT(::access(serverPath.c_str(), F_OK) == 0);
			}

			// Client owned its explicitly bound pathname
			CPPUNIT_ASSERT(::access(clientPath.c_str(), F_OK) != 0);
			// Listener is still alive and still owns this one
			CPPUNIT_ASSERT(::access(serverPath.c_str(), F_OK) == 0);
		}

		// Destruction of the listening socket removes its pathname
		CPPUNIT_ASSERT(::access(serverPath.c_str(), F_OK) != 0);
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION(UnixStreamSocket_TEST);
