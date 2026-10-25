include_directories( ${CMAKE_SOURCE_DIR} )

message("Building examples:")

set(examples
	TCP_Client
	TCP_Server
	UDP_Multicast
	UDP_Commands
	mini_ifconfig)

foreach(EX ${examples})
	message("* ${EX}")
	add_executable(${EX} ${EX}.cpp)
	target_link_libraries(${EX} network)
endforeach()
