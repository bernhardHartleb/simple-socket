#include <cppunit/extensions/HelperMacros.h>
#include <simple_socket/SocketUtils.h>
#include <simple_socket/NetworkInterface.h>

#include <algorithm>

class SocketUtils_TEST : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(SocketUtils_TEST);
	CPPUNIT_TEST(resolveHostname);
	CPPUNIT_TEST(resolveService);
	CPPUNIT_TEST(getNetworkAddresses);
	CPPUNIT_TEST(getNetworkInterfaces);
	CPPUNIT_TEST(getHardwareAddress);
	CPPUNIT_TEST_SUITE_END();

public:
	void resolveHostname()
	{
		// should work as long as one has a resonable setup
		CPPUNIT_ASSERT_EQUAL(std::string("127.0.0.1"), NET::resolveHostname("localhost"));
	}

	void resolveService()
	{
		// HTTP over TCP is known, but not via DDP
		CPPUNIT_ASSERT( NET::resolveService( "http") == 80);
		CPPUNIT_ASSERT( NET::resolveService( "http", "") == 80);
		CPPUNIT_ASSERT( NET::resolveService( "http", "ddp") == 0);
		// but there is no rtmp over TCP or UDP
		CPPUNIT_ASSERT( NET::resolveService( "rtmp") == 0);
		CPPUNIT_ASSERT( NET::resolveService( "rtmp", "udp") == 0);
		// but there is over DDP
		CPPUNIT_ASSERT( NET::resolveService( "rtmp", "ddp") == 1);
		CPPUNIT_ASSERT( NET::resolveService( "rtmp", "") == 1);
		// there is no IANA protocol named 'foo'
		CPPUNIT_ASSERT( NET::resolveService( "foo") == 0);
		// also not on other transport layers
		CPPUNIT_ASSERT( NET::resolveService( "foo", "") == 0);
	}

	void getNetworkAddresses()
	{
		const auto list = NET::getNetworkAddresses();
		CPPUNIT_ASSERT( !list.empty() );

		// assumes there is a loopback interface
		const auto loopback = std::find_if(
			list.begin(), list.end(),
			[](const NET::NetworkAddressInfo& info) {
				return info.family == NET::IPv4 &&
				       info.address == "127.0.0.1";
			});

		CPPUNIT_ASSERT( loopback != list.end() );
		CPPUNIT_ASSERT_EQUAL( std::string("lo"),        loopback->name);
		CPPUNIT_ASSERT_EQUAL( std::string("255.0.0.0"), loopback->netmask);
		CPPUNIT_ASSERT( loopback->broadcast.empty() );
		CPPUNIT_ASSERT( loopback->destination.empty() );
	}

	void getNetworkInterfaces()
	{
		const auto list = NET::getNetworkInterfaces();
		CPPUNIT_ASSERT( !list.empty() );

		// assumes there is a loopback interface
		const auto loopback = std::find_if(
			list.begin(), list.end(),
			[](const NET::NetworkInterface& iface) {
				return iface.name() == "lo";
			});

		CPPUNIT_ASSERT( loopback != list.end() );
		CPPUNIT_ASSERT_EQUAL( std::string("127.0.0.1"), loopback->getInterfaceAddress());
		CPPUNIT_ASSERT_EQUAL( std::string("0.0.0.0"),   loopback->getBroadcastAddress());
		CPPUNIT_ASSERT_EQUAL( std::string("255.0.0.0"), loopback->getNetmask());
		CPPUNIT_ASSERT_EQUAL( std::string("127.0.0.1"), loopback->getDestinationAddress());
		CPPUNIT_ASSERT_EQUAL( std::string(""),          loopback->getHardwareAddress());
		CPPUNIT_ASSERT_EQUAL( 65536,                    loopback->getMTU());
	}

	void getHardwareAddress()
	{
		const auto list = NET::getNetworkInterfaces();
		const auto interface = std::find_if(
			list.begin(), list.end(),
			[](const NET::NetworkInterface& iface) {
				return iface.name() != "lo";
			});

		// assumes there is a real itnerface with hwaddr
		CPPUNIT_ASSERT( interface != list.end() );
		const auto mac = interface->getHardwareAddress();
		CPPUNIT_ASSERT( mac.size() == 17);
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION(SocketUtils_TEST);
