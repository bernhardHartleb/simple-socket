include_directories( ${CMAKE_SOURCE_DIR} )

message("Building tests:")

# Collect all unit tests
set(Test_SRC
	TCPSocket_TEST.cpp
	UDPSocket_TEST.cpp)

if(UNIX)
	list(APPEND Test_SRC
		UnixDatagramSocket_TEST.cpp
		SocketUtils_TEST.cpp)
endif()

if(BUILD_CAN)
	list(APPEND Test_SRC
		${Test_SRC}
		CANSocket_TEST.cpp)
endif()

add_executable(UnitTester test_runner.cpp ${Test_SRC})
target_link_libraries(UnitTester network cppunit)

foreach(TB ${Test_SRC})
	get_filename_component(TestName ${TB} NAME_WE)
	message("* ${TestName}")
	add_test(${TestName} UnitTester ${TestName})
endforeach()
