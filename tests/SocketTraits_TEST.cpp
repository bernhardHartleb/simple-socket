#include <cppunit/extensions/HelperMacros.h>

#include <simple_socket/TCPSocket.h>
#include <simple_socket/UDPSocket.h>

#ifdef SIMPLE_SOCKET_HAVE_UNIX
#include <simple_socket/UnixStreamSocket.h>
#include <simple_socket/UnixDatagramSocket.h>
#endif

#ifdef SIMPLE_SOCKET_HAVE_CAN
#include <simple_socket/CANSocket.h>
#include <simple_socket/CANRawSocket.h>
#endif

#ifdef SIMPLE_SOCKET_HAVE_SCTP
#include <simple_socket/SCTPSocket.h>
#endif

#include <type_traits>

static_assert(!std::is_copy_constructible_v<NET::SimpleSocket>);
static_assert(!std::is_copy_assignable_v<NET::SimpleSocket>);
static_assert(!std::is_move_constructible_v<NET::SimpleSocket>);

static_assert(!std::is_copy_constructible_v<NET::UDPSocket>);
static_assert(!std::is_copy_assignable_v<NET::UDPSocket>);
static_assert(std::is_nothrow_move_constructible_v<NET::UDPSocket>);
static_assert(std::is_nothrow_move_assignable_v<NET::UDPSocket>);

static_assert(!std::is_copy_constructible_v<NET::TCPSocket>);
static_assert(!std::is_copy_assignable_v<NET::TCPSocket>);
static_assert(std::is_nothrow_move_constructible_v<NET::TCPSocket>);
static_assert(std::is_nothrow_move_assignable_v<NET::TCPSocket>);

#ifdef SIMPLE_SOCKET_HAVE_UNIX
static_assert(!std::is_copy_constructible_v<NET::UnixStreamSocket>);
static_assert(!std::is_copy_assignable_v<NET::UnixStreamSocket>);
static_assert(std::is_nothrow_move_constructible_v<NET::UnixStreamSocket>);
static_assert(std::is_nothrow_move_assignable_v<NET::UnixStreamSocket>);

static_assert(!std::is_copy_constructible_v<NET::UnixDatagramSocket>);
static_assert(!std::is_copy_assignable_v<NET::UnixDatagramSocket>);
static_assert(std::is_nothrow_move_constructible_v<NET::UnixDatagramSocket>);
static_assert(std::is_nothrow_move_assignable_v<NET::UnixDatagramSocket>);
#endif

#ifdef SIMPLE_SOCKET_HAVE_CAN
static_assert(!std::is_copy_constructible_v<NET::CANRawSocket>);
static_assert(!std::is_copy_assignable_v<NET::CANRawSocket>);
static_assert(std::is_nothrow_move_constructible_v<NET::CANRawSocket>);
static_assert(std::is_nothrow_move_assignable_v<NET::CANRawSocket>);
#endif

#ifdef SIMPLE_SOCKET_HAVE_SCTP
static_assert(!std::is_copy_constructible_v<NET::SCTPSocket>);
static_assert(!std::is_copy_assignable_v<NET::SCTPSocket>);
static_assert(std::is_nothrow_move_constructible_v<NET::SCTPSocket>);
static_assert(std::is_nothrow_move_assignable_v<NET::SCTPSocket>);
#endif

// Dummy
class SocketTraits_TEST : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(SocketTraits_TEST);
	CPPUNIT_TEST_SUITE_END();
};
CPPUNIT_TEST_SUITE_REGISTRATION(SocketTraits_TEST);
