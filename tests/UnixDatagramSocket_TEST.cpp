#include <cppunit/extensions/HelperMacros.h>
#include "../UnixDatagramSocket.h"

#include <unistd.h>
#include <cstring>
#include <string>

class UnixDatagramSocket_TEST : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(UnixDatagramSocket_TEST);
	CPPUNIT_TEST(testSendToReceiveFrom);
	CPPUNIT_TEST(testConnectedSendReceive);
	CPPUNIT_TEST(testTimedReceiveFrom);
	CPPUNIT_TEST(testZeroLengthDatagram);
	CPPUNIT_TEST(testSocketPathCleanup);
	CPPUNIT_TEST(testInvalidPath);
	CPPUNIT_TEST_SUITE_END();

private:
	static constexpr const char* SEND_SOCK = "unix-dgram-sender";
	static constexpr const char* REVC_SOCK = "unix-dgram-receiver";

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
	void testSendToReceiveFrom()
	{
		const std::string senderPath   = socketPath(SEND_SOCK);
		const std::string receiverPath = socketPath(REVC_SOCK);

		removeStaleSocket(senderPath);
		removeStaleSocket(receiverPath);

		NET::UnixDatagramSocket sender;
		NET::UnixDatagramSocket receiver;

		sender.bind(senderPath);
		receiver.bind(receiverPath);

		CPPUNIT_ASSERT_EQUAL(senderPath, sender.getLocalPath());
		CPPUNIT_ASSERT_EQUAL(receiverPath, receiver.getLocalPath());

		const char message[] = "Hello Unix datagram";
		sender.sendTo(message, sizeof(message), receiverPath);

		char buffer[sizeof(message)] = {};
		std::string sourcePath;

		const int received = receiver.receiveFrom(buffer, sizeof(buffer), sourcePath);

		CPPUNIT_ASSERT_EQUAL(static_cast<int>(sizeof(message)), received);
		CPPUNIT_ASSERT_EQUAL(senderPath, sourcePath);
		CPPUNIT_ASSERT(std::memcmp(message, buffer, sizeof(message)) == 0);
	}

	void testConnectedSendReceive()
	{
		const std::string clientPath = socketPath(SEND_SOCK);
		const std::string serverPath = socketPath(REVC_SOCK);

		removeStaleSocket(clientPath);
		removeStaleSocket(serverPath);

		NET::UnixDatagramSocket client;
		NET::UnixDatagramSocket server;

		client.bind(clientPath);
		server.bind(serverPath);

		// No peer exists before connect()
		CPPUNIT_ASSERT_THROW(client.getForeignPath(), NET::SocketException);

		client.connect(serverPath);
		server.connect(clientPath);

		CPPUNIT_ASSERT_EQUAL(clientPath, client.getLocalPath());
		CPPUNIT_ASSERT_EQUAL(serverPath, client.getForeignPath());
		CPPUNIT_ASSERT_EQUAL(serverPath, server.getLocalPath());
		CPPUNIT_ASSERT_EQUAL(clientPath, server.getForeignPath());

		const char request[] = "Unix datagram request";
		int sent = client.send(request, sizeof(request));

		CPPUNIT_ASSERT_EQUAL(static_cast<int>(sizeof(request)), sent);

		char buffer[64] = {};
		int received = server.timedReceive(buffer, sizeof(buffer), 1000);

		CPPUNIT_ASSERT_EQUAL(static_cast<int>(sizeof(request)), received);
		CPPUNIT_ASSERT(std::memcmp(request, buffer, sizeof(request)) == 0);

		const char reply[] = "Unix datagram reply";
		sent = server.send(reply, sizeof(reply));
		CPPUNIT_ASSERT_EQUAL(static_cast<int>(sizeof(reply)), sent);

		std::memset(buffer, 0, sizeof(buffer));
		received = client.timedReceive(buffer, sizeof(buffer), 1000);

		CPPUNIT_ASSERT_EQUAL(static_cast<int>(sizeof(reply)), received);
		CPPUNIT_ASSERT(std::memcmp(reply, buffer, sizeof(reply)) == 0);
	}

	void testTimedReceiveFrom()
	{
		const std::string senderPath   = socketPath(SEND_SOCK);
		const std::string receiverPath = socketPath(REVC_SOCK);

		removeStaleSocket(senderPath);
		removeStaleSocket(receiverPath);

		NET::UnixDatagramSocket sender;
		NET::UnixDatagramSocket receiver;

		sender.bind(senderPath);
		receiver.bind(receiverPath);

		char buffer[32];
		std::memset(buffer, 0x5a, sizeof(buffer));
		std::string sourcePath = "unchanged";

		// No datagram waiting.
		int received = receiver.timedReceiveFrom(buffer, sizeof(buffer), sourcePath, 50);

		CPPUNIT_ASSERT_EQUAL(0, received);
		// Neither the source path nor receive buffer should have been modified
		CPPUNIT_ASSERT_EQUAL(std::string("unchanged"), sourcePath);

		for (char c : buffer)
			CPPUNIT_ASSERT_EQUAL(static_cast<char>(0x5a), c);

		const char message[] = "Timed Unix datagram";
		sender.sendTo(message, sizeof(message), receiverPath);

		received = receiver.timedReceiveFrom(buffer, sizeof(buffer), sourcePath, 1000);

		CPPUNIT_ASSERT_EQUAL(static_cast<int>(sizeof(message)), received);
		CPPUNIT_ASSERT_EQUAL(senderPath, sourcePath);
		CPPUNIT_ASSERT(std::memcmp(message, buffer, sizeof(message)) == 0);
	}

	void testZeroLengthDatagram()
	{
		const std::string senderPath   = socketPath(SEND_SOCK);
		const std::string receiverPath = socketPath(REVC_SOCK);

		removeStaleSocket(senderPath);
		removeStaleSocket(receiverPath);

		NET::UnixDatagramSocket sender;
		NET::UnixDatagramSocket receiver;

		sender.bind(senderPath);
		receiver.bind(receiverPath);

		char dummy = 'X';
		sender.sendTo(&dummy, 0, receiverPath);

		std::string sourcePath;
		const int received = receiver.receiveFrom(&dummy, sizeof(dummy), sourcePath);

		// Zero bytes is a valid datagram
		CPPUNIT_ASSERT_EQUAL(0, received);
		// Sender path demonstrates that an actual datagram was consumed
		CPPUNIT_ASSERT_EQUAL(senderPath, sourcePath);
		CPPUNIT_ASSERT_EQUAL('X', dummy);
	}

	void testSocketPathCleanup()
	{
		const std::string path = socketPath("unix-dgram-cleanup");
		removeStaleSocket(path);

		CPPUNIT_ASSERT(::access(path.c_str(), F_OK) != 0);

		{
			NET::UnixDatagramSocket socket;
			socket.bind(path);

			CPPUNIT_ASSERT_EQUAL(path, socket.getLocalPath());
			CPPUNIT_ASSERT(::access(path.c_str(), F_OK) == 0);
		}

		// UnixSocket owns paths it binds and removes them during destruction
		CPPUNIT_ASSERT(::access(path.c_str(), F_OK) != 0);
	}

	void testInvalidPath()
	{
		NET::UnixDatagramSocket socket;
		const std::string path(1024, 'x');
		CPPUNIT_ASSERT_THROW(socket.bind(path), NET::SocketException);
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION(UnixDatagramSocket_TEST);
