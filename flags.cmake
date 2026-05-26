SET (C_WARNINGS "-Wall -Wextra -pedantic")
SET (C_WARNINGS "${C_WARNINGS} -Winit-self -Wparentheses -Wfloat-equal -Wno-endif-labels -Wpointer-arith -Wdeprecated")
SET (C_WARNINGS "${C_WARNINGS} -Wmissing-declarations -Wredundant-decls -Wswitch -Wswitch-enum -Wsign-promo -Wcast-qual")
SET (C_WARNINGS "${C_WARNINGS} -Wpacked -Wmissing-include-dirs -Wsign-conversion -Wdisabled-optimization -Winline")
SET (C_WARNINGS "${C_WARNINGS} -Wlogical-op")
SET (C_WARNINGS "${C_WARNINGS}" CACHE STRING "Flags to enable warnings for C file compiles")

SET (CXX_WARNINGS "-Wsign-promo -Woverloaded-virtual ${C_WARNINGS}" CACHE STRING "Flags to enable warning for c++ file compiles")

SET (COMMON_FLAGS "-fvisibility=hidden" CACHE STRING "Flags shared for all C file compiles")
SET (COMMON_LD_FLAGS "-W1,-O1,--hash-style=gnu,--as-needed,--enable-new-dtags" CACHE STRING "Flags shared for all linker runs")

SET (CMAKE_C_FLAGS   "${COMMON_FLAGS} ${C_WARNINGS}" CACHE STRING "Flags used for compiling C Files")
SET (CMAKE_CXX_FLAGS "${COMMON_FLAGS} ${CXX_WARNINGS}")
SET (CMAKE_LD_FLAGS  "${COMMON_LD_FLAGS}")

SET (CMAKE_C_FLAGS_DEBUG "-g -DDEBUG" CACHE STRING "C flags for C builds")
SET (CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG")
SET (CMAKE_LD_FLAGS_DEBUG "${COMMON_LD_FLAGS}")

SET (CMAKE_C_FLAGS_RELEASE "-DNDEBUG")
SET (CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG")
SET (CMAKE_LD_FLAGS_RELEASE "-s ${COMMON_LD_FLAGS}")
