message("Building tests:")

set(tests
	UDPSocket_TEST
	TCPSocket_TEST)

foreach(TB ${tests})

	add_executable(${TB} ${TB}.cpp)
	target_link_libraries(${TB} network)
	message("* ${TB}")
	add_test(${TB} ${CMAKE_CURRENT_BINARY_DIR}/${TB})

endforeach()
