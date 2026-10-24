cmake_minimum_required(VERSION 3.16)

project(simple-socket LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_CXX_EXTENSIONS OFF)

# Building of CAN is optional
option(BUILD_CAN "Add support for the Socket CAN layer." false)

# Building of SCTP is optional
option(BUILD_SCTP "Add support for the SCTP protocol." false)

# Building of examples is optional
option(BUILD_EXAMPLES "Switch to enable/disable building of examples." true)

# Add useful compiler warnings
add_compile_options(
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
	-Wlogical-op
	-Wcast-qual
	-Wcast-align
	-Wsign-conversion
	-Wsign-promo
	-Wstrict-null-sentinel
	-Wzero-as-null-pointer-constant
)

# Add modern linker flags
add_link_options(
	-Wl,-O1,--hash-style=gnu,--as-needed,--enable-new-dtags
)

include(CTest)

set(sources
	SimpleSocket.cpp
	SocketUtils.cpp
	InternetSocket.cpp
	TCPSocket.cpp
	UDPSocket.cpp)

if(UNIX)
	list(APPEND sources
		UnixSocket.cpp
		UnixDatagramSocket.cpp
		UnixStreamSocket.cpp)
endif()

if(BUILD_CAN)
	list(APPEND sources
		CANSocket.cpp
		CANRawSocket.cpp)
endif()

if(BUILD_SCTP)
	list(APPEND sources
		SCTPSocket.cpp)
endif()

source_group(network_src FILES ${sources})

add_library(network STATIC ${sources})
add_library(simple-socket::network ALIAS network)

target_compile_features(network PUBLIC cxx_std_17)

set_target_properties(network PROPERTIES
	CXX_EXTENSIONS OFF
	CXX_VISIBILITY_PRESET hidden
	VISIBILITY_INLINES_HIDDEN YES
)

if(BUILD_SCTP)
	find_path(SCTP_INCLUDE_DIR NAMES netinet/sctp.h)
	find_library(SCTP_LIBRARY NAMES sctp)

	if(SCTP_INCLUDE_DIR AND SCTP_LIBRARY)
		target_include_directories(network PRIVATE ${SCTP_INCLUDE_DIR})
		target_link_libraries(network PRIVATE ${SCTP_LIBRARY})
	else()
		message(FATAL_ERROR "SCTP support requested but libsctp was not found")
	endif()
endif()

if(BUILD_TESTING)
	add_subdirectory( tests )
endif()

if(BUILD_EXAMPLES)
	add_subdirectory( examples )
endif()

find_package(Doxygen)
# if doxygen is available, add 'doc' target building documentation
if(Doxygen_FOUND)
	configure_file(
		${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in
		${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY
	)
	add_custom_target(
		doc
		${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Generating API documentation using Doxygen" VERBATIM
	)
else()
	message("Doxygen not found, no 'doc' target available")
endif()
