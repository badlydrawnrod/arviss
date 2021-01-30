cmake_minimum_required(VERSION 3.13)
project(0x38_arviss
        VERSION 0.1
        DESCRIPTION "ARVISS - A RISC-V Instruction Set Simulator"
        LANGUAGES C CXX)

include(FetchContent)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

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
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

add_library(arviss decode.c smallmem.c)

add_executable(0x38_arviss arviss.c)
target_link_libraries(0x38_arviss PRIVATE arviss)

add_executable(decode_test decode_test.cpp)
target_link_libraries(decode_test PRIVATE arviss gtest_main)
