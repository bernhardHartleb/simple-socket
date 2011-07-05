include_directories( ${CMAKE_SOURCE_DIR} )

message("Building examples:")

set(examples
	TCP_Client
	TCP_Server
	UDP_Multicast)

foreach(EX ${examples})

	add_executable(${EX} ${EX}.cpp)
	target_link_libraries(${EX} network)
	message("* ${EX}")

endforeach()
