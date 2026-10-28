#include <cppunit/extensions/HelperMacros.h>
#include <simple_socket/UnixStreamSocket.h>

#include <unistd.h>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

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
		CPPUNIT_ASSERT_THROW(client.getForeignPath(), NET::SocketException);

		server.bind(serverPath);
		server.listen();

		client.bind(clientPath);
		client.connect(serverPath);

		NET::UnixStreamSocket::Handle handle = server.accept();
		CPPUNIT_ASSERT(handle);

		NET::UnixStreamSocket session(std::move(handle));

		CPPUNIT_ASSERT(!handle);
		CPPUNIT_ASSERT_EQUAL(serverPath, server.getLocalPath());
		CPPUNIT_ASSERT_EQUAL(clientPath, client.getLocalPath());
		CPPUNIT_ASSERT_EQUAL(serverPath, client.getForeignPath());
		CPPUNIT_ASSERT_EQUAL(serverPath, session.getLocalPath());
		CPPUNIT_ASSERT_EQUAL(clientPath, session.getForeignPath());

		const char message[] = "Hello Unix stream";
		const int sent = client.sendAll(message, sizeof(message));
		CPPUNIT_ASSERT_EQUAL(static_cast<int>(sizeof(message)), sent);

		char buffer[sizeof(message)] = {};
		const int received = session.receive(buffer, sizeof(message));

		CPPUNIT_ASSERT_EQUAL(sent, received);
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
		const int len = static_cast<int>(sendBuffer.size());

		for (size_t i = 0; i < sendBuffer.size(); ++i)
			sendBuffer[i] = static_cast<char>(i % 251);

		const int sent = client.sendAll(sendBuffer.data(), sendBuffer.size());
		CPPUNIT_ASSERT_EQUAL(len, sent);

		std::vector<char> receiveBuffer(sendBuffer.size());
		const int received = session.receiveAll(receiveBuffer.data(), receiveBuffer.size());
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
		NET::UnixStreamSocket::Handle handle = server.timedAccept(50);
		CPPUNIT_ASSERT(!handle);

		// Constructing a socket from invalid handle is a user error
		CPPUNIT_ASSERT_THROW(NET::UnixStreamSocket(std::move(handle)), NET::SocketException);

		NET::UnixStreamSocket client;
		client.connect(serverPath);

		handle = server.timedAccept(1000);
		CPPUNIT_ASSERT(handle);

		NET::UnixStreamSocket session(std::move(handle));
		CPPUNIT_ASSERT(!handle);
		CPPUNIT_ASSERT(!session.peerDisconnected());
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
		int received = session.timedReceive(buffer, sizeof(buffer), 50);
		CPPUNIT_ASSERT_EQUAL(0, received);
		// Timeout must not be mistaken for peer disconnect
		CPPUNIT_ASSERT(!session.peerDisconnected());

		const char message[] = "Timed Unix stream";
		const int sent = client.sendAll(message, sizeof(message));
		CPPUNIT_ASSERT_EQUAL(static_cast<int>(sizeof(message)), sent);

		received = session.timedReceive(buffer, sizeof(buffer), 1000);
		CPPUNIT_ASSERT(received > 0);
		CPPUNIT_ASSERT(received <= static_cast<int>(sizeof(message)));
		CPPUNIT_ASSERT(std::memcmp(message,buffer,static_cast<size_t>(received)) == 0);
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

		client.shutdown(NET::SimpleSocket::STOP_SEND);

		char buffer[16];
		const int received = session.timedReceive(buffer, sizeof(buffer), 1000);
		CPPUNIT_ASSERT_EQUAL(0, received);

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
