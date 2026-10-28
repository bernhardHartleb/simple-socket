message("Building simple-socket examples:")

set(SIMPLE_SOCKET_EXAMPLES
	TCP_Client
	TCP_Server
	UDP_Multicast
	UDP_Commands
	mini_ifconfig)

foreach(EX IN LISTS SIMPLE_SOCKET_EXAMPLES)
	message("* ${EX}")
	add_executable(${EX} ${EX}.cpp)
	target_link_libraries(${EX} PRIVATE simple-socket)
endforeach()
