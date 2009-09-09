cmake_minimum_required(VERSION 2.6)

# Building of SCTP is optional
option(BUILD_SCTP "Add support for the SCTP protocol." false)

# Building of tests is optional
option(BUILD_TESTS "Switch to enable/disable building of tests." true)

set(sources
	SimpleSocket.cpp
	SocketUtils.cpp
	InternetSocket.cpp
	TCPSocket.cpp
	UDPSocket.cpp)

set(tests
	UDPSocket_TEST)

if(UNIX)
	set(sources
		${sources}
		UnixSocket.cpp
		UnixDatagramSocket.cpp)
endif(UNIX)

if(BUILD_SCTP)
	set(sources
		${sources}
		SCTPSocket.cpp)
endif(BUILD_SCTP)

source_group(network_src FILES ${sources})
add_library(network STATIC ${sources})

if(BUILD_SCTP)
	target_link_libraries(network sctp)
endif(BUILD_SCTP)

if(BUILD_TESTS)
	enable_testing()
	message("Building tests:")

	foreach(TB ${tests})
		add_executable(${TB} ${TB}.cpp)
		target_link_libraries(${TB} network)
		message("* ${TB}")
		add_test(${TB} ${CMAKE_CURRENT_BINARY_DIR}/${TB})
	endforeach()
endif(BUILD_TESTS)
