#include <cppunit/extensions/HelperMacros.h>
#include <simple_socket/UnixDatagramSocket.h>

#include <unistd.h>
#include <cstring>
#include <string>

using NET::IOResult;
using NET::IOStatus;
using NET::ConnectResult;

class UnixDatagramSocket_TEST : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(UnixDatagramSocket_TEST);
	CPPUNIT_TEST(testSendToReceiveFrom);
	CPPUNIT_TEST(testConnectedSendReceive);
	CPPUNIT_TEST(testTimedReceiveFrom);
	CPPUNIT_TEST(testZeroLengthDatagram);
	CPPUNIT_TEST(testNonBlockingSendToWouldBlock);
	CPPUNIT_TEST(testSocketPathCleanup);
	CPPUNIT_TEST(testInvalidPath);
	CPPUNIT_TEST(testAdoptNativeHandle);
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

public:
	void testSendToReceiveFrom()
	{
		const std::string senderPath   = socketPath(SEND_SOCK);
		const std::string receiverPath = socketPath(REVC_SOCK);

		NET::UnixDatagramSocket sender;
		NET::UnixDatagramSocket receiver;

		sender.bind(senderPath);
		receiver.bind(receiverPath);

		CPPUNIT_ASSERT_EQUAL(senderPath, sender.getLocalPath());
		CPPUNIT_ASSERT_EQUAL(receiverPath, receiver.getLocalPath());

		const char message[] = "Hello Unix datagram";
		IOResult sent = sender.sendTo(message, sizeof(message), receiverPath);
		CPPUNIT_ASSERT(sent);

		char buffer[sizeof(message)] = {};
		std::string sourcePath;

		IOResult recv = receiver.receiveFrom(buffer, sizeof(buffer), sourcePath);
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, recv.status);
		CPPUNIT_ASSERT_EQUAL(sizeof(message), recv.bytes);
		CPPUNIT_ASSERT_EQUAL(senderPath, sourcePath);
		CPPUNIT_ASSERT(std::memcmp(message, buffer, sizeof(message)) == 0);
	}

	void testConnectedSendReceive()
	{
		const std::string clientPath = socketPath(SEND_SOCK);
		const std::string serverPath = socketPath(REVC_SOCK);

		NET::UnixDatagramSocket client;
		NET::UnixDatagramSocket server;

		client.bind(clientPath);
		server.bind(serverPath);

		// No peer exists before connect()
		CPPUNIT_ASSERT_EQUAL(std::string(), client.getForeignPath());

		ConnectResult res;
		res = client.connect(serverPath);
		CPPUNIT_ASSERT_EQUAL(ConnectResult::Connected, res);
		res = server.connect(clientPath);
		CPPUNIT_ASSERT_EQUAL(ConnectResult::Connected, res);

		CPPUNIT_ASSERT_EQUAL(clientPath, client.getLocalPath());
		CPPUNIT_ASSERT_EQUAL(serverPath, client.getForeignPath());
		CPPUNIT_ASSERT_EQUAL(serverPath, server.getLocalPath());
		CPPUNIT_ASSERT_EQUAL(clientPath, server.getForeignPath());

		const char request[] = "Unix datagram request";
		IOResult sent = client.send(request, sizeof(request));
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, sent.status);
		CPPUNIT_ASSERT_EQUAL(sizeof(request), sent.bytes);

		char buffer[64] = {};
		IOResult recv = server.timedReceive(buffer, sizeof(buffer), 1000);
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, recv.status);
		CPPUNIT_ASSERT_EQUAL(sizeof(request), recv.bytes);
		CPPUNIT_ASSERT(std::memcmp(request, buffer, sizeof(request)) == 0);

		const char reply[] = "Unix datagram reply";
		sent = server.send(reply, sizeof(reply));
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, sent.status);
		CPPUNIT_ASSERT_EQUAL(sizeof(reply), sent.bytes);

		std::memset(buffer, 0, sizeof(buffer));
		recv = client.timedReceive(buffer, sizeof(buffer), 1000);
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, recv.status);
		CPPUNIT_ASSERT_EQUAL(sizeof(reply), recv.bytes);
		CPPUNIT_ASSERT(std::memcmp(reply, buffer, sizeof(reply)) == 0);
	}

	void testTimedReceiveFrom()
	{
		const std::string senderPath   = socketPath(SEND_SOCK);
		const std::string receiverPath = socketPath(REVC_SOCK);

		NET::UnixDatagramSocket sender;
		NET::UnixDatagramSocket receiver;

		sender.bind(senderPath);
		receiver.bind(receiverPath);

		char buffer[32];
		std::memset(buffer, 0x5a, sizeof(buffer));
		std::string sourcePath = "unchanged";

		// No datagram waiting.
		IOResult recv = receiver.timedReceiveFrom(buffer, sizeof(buffer), sourcePath, 50);
		CPPUNIT_ASSERT_EQUAL(IOStatus::Timeout, recv.status);
		CPPUNIT_ASSERT_EQUAL(0ul, recv.bytes);
		// Neither the source path nor receive buffer should have been modified
		CPPUNIT_ASSERT_EQUAL(std::string("unchanged"), sourcePath);

		for (char c : buffer)
			CPPUNIT_ASSERT_EQUAL(static_cast<char>(0x5a), c);

		const char message[] = "Timed Unix datagram";
		IOResult sent = sender.sendTo(message, sizeof(message), receiverPath);
		CPPUNIT_ASSERT(sent);

		recv = receiver.timedReceiveFrom(buffer, sizeof(buffer), sourcePath, 1000);
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, recv.status);
		CPPUNIT_ASSERT_EQUAL(sizeof(message), recv.bytes);
		CPPUNIT_ASSERT_EQUAL(senderPath, sourcePath);
		CPPUNIT_ASSERT(std::memcmp(message, buffer, sizeof(message)) == 0);
	}

	void testZeroLengthDatagram()
	{
		const std::string senderPath   = socketPath(SEND_SOCK);
		const std::string receiverPath = socketPath(REVC_SOCK);

		NET::UnixDatagramSocket sender;
		NET::UnixDatagramSocket receiver;

		sender.bind(senderPath);
		receiver.bind(receiverPath);

		char dummy = 'X';
		IOResult sent = sender.sendTo(&dummy, 0, receiverPath);
		CPPUNIT_ASSERT(sent);

		std::string sourcePath;
		IOResult recv = receiver.receiveFrom(&dummy, sizeof(dummy), sourcePath);
		// Zero bytes is a valid datagram
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, recv.status);
		CPPUNIT_ASSERT_EQUAL(0ul, recv.bytes);
		// Sender path demonstrates that an actual datagram was consumed
		CPPUNIT_ASSERT_EQUAL(senderPath, sourcePath);
		CPPUNIT_ASSERT_EQUAL('X', dummy);
	}

	void testSocketPathCleanup()
	{
		const std::string path = socketPath("unix-dgram-cleanup");
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
		const std::string badPath(1024, 'x');
		const std::string missingPath = socketPath("unix-dgram-missing");
		char buffer[] = "asdf";

		NET::UnixDatagramSocket socket;
		CPPUNIT_ASSERT_THROW(socket.bind(badPath), NET::SocketException);
		CPPUNIT_ASSERT_THROW(auto t = socket.sendTo(buffer, sizeof(buffer), badPath), NET::SocketException);

		// no exception when sending datagrams to missing socket file
		IOResult sent = socket.sendTo(buffer, sizeof(buffer), missingPath);
		CPPUNIT_ASSERT_EQUAL(IOStatus::DestinationUnavailable, sent.status);
	}

	void testAdoptNativeHandle()
	{
		const std::string serverPath = socketPath("unix-dgram-systemd");
		NET::UnixDatagramSocket systemdOwned;
		systemdOwned.bind(serverPath);

		// dup() models systemd retaining its descriptor while passing another
		// descriptor for the same listening socket to the service.
		const int passedFd = systemdOwned.duplicateNativeHandle();
		CPPUNIT_ASSERT(passedFd >= 0);

		{
			NET::UnixDatagramSocket socket = NET::UnixDatagramSocket::adoptNativeHandle(passedFd);
			CPPUNIT_ASSERT(socket.valid());
			CPPUNIT_ASSERT_EQUAL(passedFd, socket.nativeHandle());
			CPPUNIT_ASSERT_EQUAL(serverPath, socket.getLocalPath());

			NET::UnixDatagramSocket peer;
			ConnectResult res = peer.connect(serverPath);
			CPPUNIT_ASSERT_EQUAL(ConnectResult::Connected, res);

			char buffer[] = "asdf";
			IOResult sent = peer.send( buffer, sizeof(buffer));
			CPPUNIT_ASSERT(sent);

			IOResult recv = socket.timedReceive( buffer, sizeof(buffer), 50);
			CPPUNIT_ASSERT_EQUAL(IOStatus::Success, recv.status);
			CPPUNIT_ASSERT_EQUAL(sizeof(buffer), recv.bytes);
		}

		// The service-side descriptor was owned by the wrapper and is closed,
		// while the systemd-owned socket and pathname remain alive.
		CPPUNIT_ASSERT_THROW(auto t = NET::UnixDatagramSocket::adoptNativeHandle(passedFd),  NET::SocketException);
		CPPUNIT_ASSERT(systemdOwned.valid());
		CPPUNIT_ASSERT(::access(serverPath.c_str(), F_OK) == 0);
	}

	void testNonBlockingSendToWouldBlock()
	{
		const std::string senderPath = socketPath("unix-nonblocking-sender");
		const std::string receiverPath = socketPath("unix-nonblocking-receiver");

		NET::UnixDatagramSocket sender;
		NET::UnixDatagramSocket receiver;

		sender.bind(senderPath);
		receiver.bind(receiverPath);
		sender.setBlocking(false);
		CPPUNIT_ASSERT(!sender.isBlocking());

		const char payload[] = "queued datagram";
		bool sawWouldBlock = false;
		for(unsigned i = 0; i < 1024; ++i) // bounded: receiver never reads
		{
			const IOResult result = sender.sendTo(payload, sizeof(payload), receiverPath);
			if(result.status == IOStatus::WouldBlock)
			{
				CPPUNIT_ASSERT_EQUAL(size_t(0), result.bytes);
				sawWouldBlock = true;
				break;
			}
			CPPUNIT_ASSERT_EQUAL(IOStatus::Success, result.status);
			CPPUNIT_ASSERT_EQUAL(sizeof(payload), result.bytes);
		}
		CPPUNIT_ASSERT(sawWouldBlock);
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION(UnixDatagramSocket_TEST);
