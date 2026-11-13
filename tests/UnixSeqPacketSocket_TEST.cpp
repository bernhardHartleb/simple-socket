#include <cppunit/extensions/HelperMacros.h>
#include <simple_socket/UnixSeqPacketSocket.h>

#include <unistd.h>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

using NET::IOResult;
using NET::IOStatus;
using NET::ConnectResult;

class UnixSeqPacketSocket_TEST : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(UnixSeqPacketSocket_TEST);
	CPPUNIT_TEST(testConnectAcceptAndTransfer);
	CPPUNIT_TEST(testTimedAccept);
	CPPUNIT_TEST(testTimedReceive);
	CPPUNIT_TEST(testGracefulShutdown);
	CPPUNIT_TEST(testAdoptNativeHandle);
	CPPUNIT_TEST_SUITE_END();

private:
	static constexpr const char* SERVER_SOCK = "unix-seqpacket-server";
	static constexpr const char* CLIENT_SOCK = "unix-seqpacket-client";

	static std::string socketPath(const char* name)
	{
		return std::string("/tmp/simple-socket-")
		     + std::to_string(static_cast<long long>(::getpid()))
		     + "-" + name + ".sock";
	}

public:
	void testConnectAcceptAndTransfer()
	{
		const std::string serverPath = socketPath(SERVER_SOCK);
		const std::string clientPath = socketPath(CLIENT_SOCK);

		NET::UnixSeqPacketSocket server;
		NET::UnixSeqPacketSocket client;

		ConnectResult res = client.connect(serverPath);
		CPPUNIT_ASSERT_EQUAL(ConnectResult::Unreachable, res);
		// No peer exists before connect()
		CPPUNIT_ASSERT_EQUAL(std::string(), client.getForeignPath());

		server.bind(serverPath);
		server.listen();

		client.bind(clientPath);
		res = client.connect(serverPath);
		CPPUNIT_ASSERT_EQUAL(ConnectResult::Connected, res);

		NET::UnixSeqPacketSocket session = *server.accept();
		CPPUNIT_ASSERT_EQUAL(serverPath, server.getLocalPath());
		CPPUNIT_ASSERT_EQUAL(clientPath, client.getLocalPath());
		CPPUNIT_ASSERT_EQUAL(serverPath, client.getForeignPath());
		CPPUNIT_ASSERT_EQUAL(serverPath, session.getLocalPath());
		CPPUNIT_ASSERT_EQUAL(clientPath, session.getForeignPath());

		const char message[] = "Hello Unix seqpacket";
		IOResult sent = client.send(message, sizeof(message));
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, sent.status);
		CPPUNIT_ASSERT_EQUAL(sizeof(message), sent.bytes);

		char buffer[sizeof(message)] = {};
		IOResult recv = session.receive(buffer, sizeof(message));
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, recv.status);
		CPPUNIT_ASSERT_EQUAL(sizeof(message), recv.bytes);
		CPPUNIT_ASSERT(std::memcmp(message, buffer, sizeof(message)) == 0);
	}

	void testTimedAccept()
	{
		const std::string serverPath = socketPath(SERVER_SOCK);
		NET::UnixSeqPacketSocket server;

		server.bind(serverPath);
		server.listen();

		// Nobody is connecting yet
		auto opt = server.timedAccept(50);
		CPPUNIT_ASSERT(!opt);

		NET::UnixSeqPacketSocket client;
		ConnectResult res = client.connect(serverPath);
		CPPUNIT_ASSERT_EQUAL(ConnectResult::Connected, res);

		opt = server.timedAccept(1000);
		CPPUNIT_ASSERT(opt);
		CPPUNIT_ASSERT(opt->valid());

		NET::UnixSeqPacketSocket session(std::move(*opt));
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
		NET::UnixSeqPacketSocket server;
		NET::UnixSeqPacketSocket client;

		server.bind(serverPath);
		server.listen();
		ConnectResult res = client.connect(serverPath);
		CPPUNIT_ASSERT_EQUAL(ConnectResult::Connected, res);

		NET::UnixSeqPacketSocket session(*server.accept());

		char buffer[64] = {};
		// Connection exists, but no data is waiting
		IOResult recv = session.timedReceive(buffer, sizeof(buffer), 50);
		CPPUNIT_ASSERT_EQUAL(IOStatus::Timeout, recv.status);
		CPPUNIT_ASSERT_EQUAL(0ul, recv.bytes);
		// Timeout must not be mistaken for peer disconnect
		CPPUNIT_ASSERT(!session.peerDisconnected());

		const char message[] = "Timed Unix seqpacket";
		IOResult sent = client.send(message, sizeof(message));
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, sent.status);
		CPPUNIT_ASSERT_EQUAL(sizeof(message), sent.bytes);

		recv = session.timedReceive(buffer, sizeof(buffer), 1000);
		CPPUNIT_ASSERT_EQUAL(IOStatus::Success, recv.status);
		CPPUNIT_ASSERT_EQUAL(sizeof(message), recv.bytes);
		CPPUNIT_ASSERT(std::memcmp(message,buffer, recv.bytes) == 0);
		CPPUNIT_ASSERT(!session.peerDisconnected());
	}

	void testGracefulShutdown()
	{
		const std::string serverPath = socketPath("unix-seqpacket-shutdown");
		NET::UnixSeqPacketSocket server;
		NET::UnixSeqPacketSocket client;

		server.bind(serverPath);
		server.listen();

		ConnectResult res = client.connect(serverPath);
		CPPUNIT_ASSERT_EQUAL(ConnectResult::Connected, res);

		NET::UnixSeqPacketSocket session(*server.accept());
		CPPUNIT_ASSERT(!session.peerDisconnected());

		client.shutdown(NET::STOP_SEND);

		char buffer[16];
		IOResult recv = session.timedReceive(buffer, sizeof(buffer), 1000);
		CPPUNIT_ASSERT_EQUAL(IOStatus::PeerDisconnected, recv.status);
		CPPUNIT_ASSERT_EQUAL(0ul, recv.bytes);
		CPPUNIT_ASSERT(session.peerDisconnected());
	}

	void testAdoptNativeHandle()
	{
		const std::string serverPath = socketPath("unix-seqpacket-systemd");
		NET::UnixSeqPacketSocket systemdOwned;
		systemdOwned.bind(serverPath);
		systemdOwned.listen();

		// dup() models systemd retaining its descriptor while passing another
		// descriptor for the same listening socket to the service.
		const int passedFd = systemdOwned.duplicateNativeHandle();
		CPPUNIT_ASSERT(passedFd >= 0);

		{
			NET::UnixSeqPacketSocket listener = NET::UnixSeqPacketSocket::adoptNativeHandle(passedFd);
			CPPUNIT_ASSERT(listener.valid());
			CPPUNIT_ASSERT_EQUAL(passedFd, listener.nativeHandle());
			CPPUNIT_ASSERT_EQUAL(serverPath, listener.getLocalPath());

			NET::UnixSeqPacketSocket client;
			ConnectResult res = client.connect(serverPath);
			CPPUNIT_ASSERT_EQUAL(ConnectResult::Connected, res);
			NET::UnixSeqPacketSocket session = *listener.accept();
			CPPUNIT_ASSERT(!session.peerDisconnected());
		}

		// The service-side descriptor was owned by the wrapper and is closed,
		// while the systemd-owned listener and pathname remain alive.
		CPPUNIT_ASSERT_THROW(auto t = NET::UnixSeqPacketSocket::adoptNativeHandle(passedFd),  NET::SocketException);
		CPPUNIT_ASSERT(systemdOwned.valid());
		CPPUNIT_ASSERT(::access(serverPath.c_str(), F_OK) == 0);
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION(UnixSeqPacketSocket_TEST);
