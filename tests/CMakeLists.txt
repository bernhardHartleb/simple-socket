include_directories( ${CMAKE_SOURCE_DIR} )

message("Building tests:")

# Collect all unit tests
file(GLOB Test_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*TEST.cpp")

add_executable(UnitTester test_runner.cpp ${Test_SRC})
target_link_libraries(UnitTester network cppunit)

foreach(TB ${Test_SRC})

	get_filename_component(TestName ${TB} NAME_WE)
	message("* ${TestName}")
	add_test(${TestName} UnitTester ${TestName})

endforeach()
