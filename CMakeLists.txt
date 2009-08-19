cmake_minimum_required(VERSION 2.6)

# Make building of SCTP optional
set(BUILD_SCTP true CACHE BOOL "Add support for the SCTP protocol.")

set(sources
	SimpleSocket.cpp
	SocketUtils.cpp
	InternetSocket.cpp
	TCPSocket.cpp
	UDPSocket.cpp
	UnixSocket.cpp
	UnixDatagramSocket.cpp)

if(BUILD_SCTP)
	set(sources
		${sources}
		SCTPSocket.cpp)
endif(BUILD_SCTP)

source_group(network_src FILES ${sources})

add_library(network STATIC ${sources})

if(FALSE)
	message("Building network tests:")

	set(TESTS
		UDPSocket_TEST)

	foreach(TEST ${TESTS})

		add_executable(${TEST} ${TEST}.cpp)
		target_link_libraries(${TEST} network)
		message("* ${TEST}")
		add_test(${TEST} ${CMAKE_CURRENT_BINARY_DIR}/${TEST})

	endforeach()
endif(FALSE)
