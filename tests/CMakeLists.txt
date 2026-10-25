message("Building tests:")

# Collect all unit tests
set(Test_SRC
	TCPSocket_TEST.cpp
	UDPSocket_TEST.cpp
	SocketUtils_TEST.cpp)

if(UNIX)
	list(APPEND Test_SRC
		UnixDatagramSocket_TEST.cpp
		UnixStreamSocket_TEST.cpp)
endif()

if(BUILD_CAN)
	list(APPEND Test_SRC
		CANSocket_TEST.cpp)
endif()

find_library(CPPUNIT_LIBRARY NAMES cppunit)

add_executable(UnitTester test_runner.cpp ${Test_SRC})
target_link_libraries(UnitTester PRIVATE network ${CPPUNIT_LIBRARY})

foreach(TB ${Test_SRC})
	get_filename_component(TestName ${TB} NAME_WE)
	message("* ${TestName}")
	add_test(${TestName} UnitTester ${TestName})
endforeach()
