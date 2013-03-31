#include <cppunit/extensions/HelperMacros.h>
#include "../SocketUtils.h"

class SocketUtils_TEST : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE( SocketUtils_TEST);
	CPPUNIT_TEST( resolveService );
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp() {}
	void tearDown() {}

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
};

CPPUNIT_TEST_SUITE_REGISTRATION( SocketUtils_TEST );
