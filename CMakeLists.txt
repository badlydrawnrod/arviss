cmake_minimum_required(VERSION 3.13)
project(0x38_arviss
        VERSION 0.1
        DESCRIPTION "ARVISS - A RISC-V Instruction Set Simulator"
        LANGUAGES C CXX)

include(FetchContent)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Use gtest.
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.10.0
)
FetchContent_GetProperties(googletest)
if (NOT googletest_POPULATED)
    FetchContent_Populate(googletest)
    add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
endif ()

# Prevent google test from overriding the parent project's compiler/linker settings on Windows
#set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

add_library(arviss arviss.c smallmem.c rounding_modes.h opcodes.h)
target_include_directories(arviss
        PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
        "$<INSTALL_INTERFACE:include>"
        )

install(TARGETS arviss
        EXPORT arvissTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
        )

install(FILES arviss.h smallmem.h memory.h result.h DESTINATION include/arviss)

install(EXPORT arvissTargets
        FILE arvissTargets.cmake
        NAMESPACE arviss::
        DESTINATION lib/cmake/arviss
        )

add_executable(simple_demo simple_demo.c rounding_modes.h)
target_link_libraries(simple_demo PRIVATE arviss)

add_executable(decode_test decode_test.cpp rounding_modes.h opcodes.h)
target_link_libraries(decode_test PRIVATE arviss gtest_main)
