cmake_minimum_required(VERSION 2.6)
set(sources
	SimpleSocket.cpp
	SocketUtils.cpp
	InternetSocket.cpp
	TCPSocket.cpp
	UDPSocket.cpp
	UnixSocket.cpp
	UnixDatagramSocket.cpp
	SCTPSocket.cpp)

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
