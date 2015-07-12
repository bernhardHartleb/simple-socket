if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  SET (USING_CLANG TRUE)
else()
  SET (USING_CLANG FALSE)
endif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")

SET (C_WARNINGS "-Wall -Wextra -Winit-self -pedantic -Wparentheses -Wfloat-equal -Wno-endif-labels -Wpointer-arith -Wdeprecated")
SET (C_WARNINGS "${C_WARNINGS} -Wmissing-declarations -Wredundant-decls -Wswitch -Wswitch-enum -Wsign-promo -Wconversion")
SET (C_WARNINGS "${C_WARNINGS} -Wcast-qual -Wmissing-include-dirs")
if(NOT USING_CLANG)
  SET( C_WARNINGS "${C_WARNINGS} -Wlogical-op")
endif()
SET (C_WARNINGS ${C_WARNINGS} CACHE STRING "Flags to enable warnings for C file compiles")

SET (CXX_WARNINGS "-Wsign-promo -Woverloaded-virtual ${C_WARNINGS}" CACHE STRING "Flags to enable warning for c++ file compiles")

SET (OPTIMIZE_C_FLAGS "-O2 -fivopts -ftree-loop-linear -fvisibility=hidden" CACHE STRING "Flags used to optimize C files")
SET (SHARED_C_FLAGS "-pipe -fvisibility-inlines-hidden")
if(NOT USING_CLANG)
  MESSAGE("-- Adding fno-ident")
  SET( SHARED_C_FLAGS "${SHARED_C_FLAGS} -fno-ident")
endif()
SET (SHARED_C_FLAGS ${SHARED_C_FLAGS} CACHE STRING "Flags shared for all C file compiles")

SET (SHARED_CXX_FLAGS "-std=c++11" CACHE STRING "Flags shared for all C++ file compiles")
SET (SHARED_LD_FLAGS "-W1,-O1,--hash-style=gnu,--sort-common,--as-needed,--enable-new-dtags" CACHE STRING "Flags shared for all linker runs")

SET (CMAKE_C_FLAGS "${SHARED_C_FLAGS} ${C_WARNINGS}" CACHE STRING "Flags used for compiling C Files")
SET (CMAKE_CXX_FLAGS "${SHARED_CXX_FLAGS} ${SHARED_C_FLAGS} ${C_WARNINGS} ${CXX_WARNINGS}")
SET (CMAKE_LD_FLAGS	"${SHARED_LD_FLAGS}")

SET (CMAKE_C_FLAGS_DEBUG "-ggdb ${CMAKE_C_FLAGS} -DDEBUG" CACHE STRING "C flags for C builds")
SET (CMAKE_CXX_FLAGS_DEBUG "-ggdb ${CMAKE_CXX_FLAGS} -DDEBUG")
SET (CMAKE_LD_FLAGS_DEBUG "${SHARED_LD_FLAGS}")

SET (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} ${OPTIMIZE_C_FLAGS} -DNDEBUG")
SET (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} ${OPTIMIZE_C_FLAGS} -DNDEBUG")
SET (CMAKE_LD_FLAGS_RELEASE "-s ${CMAKE_LD_FLAGS}")