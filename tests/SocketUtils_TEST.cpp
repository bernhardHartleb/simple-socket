#include <cppunit/extensions/HelperMacros.h>
#include "../SocketUtils.h"
#include <iostream>

class SocketUtils_TEST : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE( SocketUtils_TEST);
	CPPUNIT_TEST( resolveHostname );
	CPPUNIT_TEST( resolveService );
	CPPUNIT_TEST( getInterfaceAddress );
	CPPUNIT_TEST( getBroadcastAddress );
	CPPUNIT_TEST( getNetmask );
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp() {}
	void tearDown() {}

	// should work as long as one has a resonable setup
	void resolveHostname() {
		CPPUNIT_ASSERT_EQUAL(
			std::string("127.0.0.1"),
			NET::resolveHostname( "localhost"));
	}

	void resolveService() {
		// HTTP over TCP is known, also over UDP, but not via DDP
		CPPUNIT_ASSERT( NET::resolveService( "http") == 80);
		CPPUNIT_ASSERT( NET::resolveService( "http", "") == 80);
		CPPUNIT_ASSERT( NET::resolveService( "http", "udp") == 80);
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

	void getInterfaceAddress() {
		CPPUNIT_ASSERT_EQUAL(
			NET::getInterfaceAddress("lo"),
			std::string("127.0.0.1"));
	}

	void getBroadcastAddress() {
		CPPUNIT_ASSERT_EQUAL(
			NET::getBroadcastAddress("lo"),
			std::string("127.255.255.255"));
	}

	void getNetmask() {
		CPPUNIT_ASSERT_EQUAL(
			NET::getNetmask("lo"),
			std::string("255.0.0.0"));
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( SocketUtils_TEST );
