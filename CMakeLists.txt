cmake_minimum_required(VERSION 3.16)

project(simple-socket
	VERSION 1.5.0
	LANGUAGES CXX)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
	set(SIMPLE_SOCKET_TOP_LEVEL ON)
else()
	set(SIMPLE_SOCKET_TOP_LEVEL OFF)
endif()

option(SIMPLE_SOCKET_ENABLE_CAN "Add support for the Socket CAN layer." OFF)
option(SIMPLE_SOCKET_ENABLE_SCTP "Add support for the SCTP protocol." OFF)
option(SIMPLE_SOCKET_BUILD_EXAMPLES "Building of example progams." ${SIMPLE_SOCKET_TOP_LEVEL})
option(SIMPLE_SOCKET_BUILD_DOCS "Enable the Doxygen documentation target." ${SIMPLE_SOCKET_TOP_LEVEL})

# BUILD_TESTING uses conventional CMake option
if(SIMPLE_SOCKET_TOP_LEVEL)
	include(CTest)
endif()

include(GNUInstallDirs)


### Sources

set(SIMPLE_SOCKET_SOURCES
	src/SimpleSocket.cpp
	src/SockAddress.cpp
	src/SocketUtils.cpp
	src/NetworkInterface.cpp
	src/InternetSocket.cpp
	src/TCPSocket.cpp
	src/UDPSocket.cpp)

set(SIMPLE_SOCKET_HAVE_UNIX OFF)

if(UNIX)
	set(SIMPLE_SOCKET_HAVE_UNIX ON)

	list(APPEND SIMPLE_SOCKET_SOURCES
		src/UnixSocket.cpp
		src/UnixDatagramSocket.cpp
		src/UnixStreamSocket.cpp)
endif()

set(SIMPLE_SOCKET_HAVE_CAN OFF)

if(SIMPLE_SOCKET_ENABLE_CAN)
	set(SIMPLE_SOCKET_HAVE_CAN ON)

	list(APPEND SIMPLE_SOCKET_SOURCES
		src/CANSocket.cpp
		src/CANRawSocket.cpp)
endif()

set(SIMPLE_SOCKET_HAVE_SCTP OFF)

if(SIMPLE_SOCKET_ENABLE_SCTP)
	find_path(SCTP_INCLUDE_DIR NAMES netinet/sctp.h)
	find_library(SCTP_LIBRARY NAMES sctp)

	if(SCTP_INCLUDE_DIR AND SCTP_LIBRARY)
		set(SIMPLE_SOCKET_HAVE_SCTP ON)

		list(APPEND SIMPLE_SOCKET_SOURCES
			src/SCTPSocket.cpp)
	else()
		message(FATAL_ERROR "SCTP support requested but libsctp was not found")
	endif()
endif()


### Library

add_library(simple-socket STATIC ${SIMPLE_SOCKET_SOURCES})

# Compatibility for old unnamespaced target
if(NOT TARGET network)
	add_library(network ALIAS simple-socket)
endif()

target_include_directories(simple-socket
	PUBLIC
		$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
	PRIVATE
		${PROJECT_SOURCE_DIR}/include/simple_socket
		${PROJECT_SOURCE_DIR}/src
)

set_target_properties(simple-socket PROPERTIES
	CXX_EXTENSIONS OFF
	CXX_VISIBILITY_PRESET hidden
	VISIBILITY_INLINES_HIDDEN YES
	POSITION_INDEPENDENT_CODE ON
)

target_compile_features(simple-socket PUBLIC cxx_std_17)
target_compile_options(simple-socket
	PRIVATE
	-Wall
	-Wextra
	-pedantic
	-Wfloat-equal
	-Wdeprecated
	-Wswitch-enum
	-Winit-self
	-Wredundant-decls
	-Wmissing-declarations
	-Wmissing-include-dirs
	-Wpacked
	-Wdisabled-optimization
	-Wcast-qual
	-Wcast-align
	-Wsign-conversion
	-Wsign-promo
	-Wzero-as-null-pointer-constant
)

if(SIMPLE_SOCKET_HAVE_UNIX)
	target_compile_definitions(simple-socket PUBLIC SIMPLE_SOCKET_HAVE_UNIX)
endif()

if(SIMPLE_SOCKET_HAVE_CAN)
	target_compile_definitions(simple-socket PUBLIC SIMPLE_SOCKET_HAVE_CAN)
endif()

if(SIMPLE_SOCKET_HAVE_SCTP)
	target_compile_definitions(simple-socket PUBLIC SIMPLE_SOCKET_HAVE_SCTP)
	target_include_directories(simple-socket PRIVATE ${SCTP_INCLUDE_DIR})
	target_link_libraries(simple-socket PRIVATE ${SCTP_LIBRARY})
endif()


### Other targets

if(SIMPLE_SOCKET_TOP_LEVEL AND BUILD_TESTING)
	add_subdirectory(tests)
endif()

if(SIMPLE_SOCKET_BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()

if(SIMPLE_SOCKET_BUILD_DOCS)
	find_package(Doxygen QUIET)
	if(Doxygen_FOUND)
		configure_file(
			${PROJECT_SOURCE_DIR}/doc/Doxyfile.in
			${PROJECT_BINARY_DIR}/Doxyfile @ONLY
		)
		add_custom_target(
			doc
			COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_BINARY_DIR}/Doxyfile
			WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
			COMMENT "Generating simple-socket API documentation"
			VERBATIM
		)
	endif()
endif()
