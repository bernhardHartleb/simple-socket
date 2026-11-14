message("Building simple-socket examples:")

set(SIMPLE_SOCKET_EXAMPLES
	TCP_Client
	TCP_Client_Reconnect
	TCP_Server
	UDP_Multicast
	UDP_Commands
	mini_ifconfig)

if(SIMPLE_SOCKET_HAVE_EPOLL)
	list(APPEND SIMPLE_SOCKET_EXAMPLES
		EventHandler_Timer
		UDP_Event_Receiver
		TCP_Event_Server)
endif()

foreach(EX IN LISTS SIMPLE_SOCKET_EXAMPLES)
	message("* ${EX}")
	add_executable(${EX} ${EX}.cpp)
	target_link_libraries(${EX} PRIVATE simple-socket)
endforeach()
