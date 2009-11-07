include_directories( ${CMAKE_SOURCE_DIR} )

message("Building examples:")

set(tests
	TCP_Client
	TCP_Server)

foreach(TB ${tests})

	add_executable(${TB} ${TB}.cpp)
	target_link_libraries(${TB} network)
	message("* ${TB}")

endforeach()
