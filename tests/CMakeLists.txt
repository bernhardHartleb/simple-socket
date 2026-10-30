message("Building simple-socket tests:")

find_package(PkgConfig REQUIRED)
pkg_check_modules(CPPUNIT REQUIRED IMPORTED_TARGET cppunit)

set(SIMPLE_SOCKET_TESTS
	TCPSocket_TEST.cpp
	UDPSocket_TEST.cpp
	SocketUtils_TEST.cpp
	SocketTraits_TEST.cpp)

if(SIMPLE_SOCKET_HAVE_UNIX)
	list(APPEND SIMPLE_SOCKET_TESTS
		UnixDatagramSocket_TEST.cpp
		UnixStreamSocket_TEST.cpp)
endif()

if(SIMPLE_SOCKET_HAVE_CAN)
	list(APPEND SIMPLE_SOCKET_TESTS
		CANSocket_TEST.cpp)
endif()

#if(SIMPLE_SOCKET_HAVE_SCTP)
#	list(APPEND SIMPLE_SOCKET_TESTS
#		SCTPSocket_TEST.cpp)
#endif()

add_executable(UnitTester test_runner.cpp ${SIMPLE_SOCKET_TESTS})
target_link_libraries(UnitTester PRIVATE simple-socket PkgConfig::CPPUNIT)

foreach(TS IN LISTS SIMPLE_SOCKET_TESTS)
	get_filename_component(TN ${TS} NAME_WE)
	message("* ${TN}")
	add_test(${TN} UnitTester ${TN})
endforeach()
