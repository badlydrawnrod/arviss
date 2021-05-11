cmake_minimum_required(VERSION 3.13)
project(0x38_arviss
        VERSION 0.1
        DESCRIPTION "ARVISS - A RISC-V Instruction Set Simulator"
        LANGUAGES C CXX)

enable_testing()

include(FetchContent)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Use gtest. See: https://github.com/google/googletest/blob/master/googletest/README.md
FetchContent_Declare(
        googletest
        # Specify the commit you depend on and update it regularly.
        URL https://github.com/google/googletest/archive/refs/tags/v1.10.x.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Arviss - the library.
add_library(arviss STATIC arviss.c rounding_modes.h opcodes.h)
target_include_directories(arviss
        PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
        "$<INSTALL_INTERFACE:include>"
        )
target_compile_options(arviss PRIVATE
        $<$<OR:$<C_COMPILER_ID:Clang>,$<C_COMPILER_ID:AppleClang>,$<C_COMPILER_ID:GNU>>:
        -Wall -Werror>
        $<$<C_COMPILER_ID:MSVC>:
        /WX>)

# Arviss tests.
add_executable(decode_test decode_test.cpp smallmem.c smallmem.h)
target_link_libraries(decode_test PRIVATE arviss gtest_main)
add_test(decode_test decode_test)

# Run "hello world" via Arviss.
add_executable(run_hello run_hello.c mem.c mem.h)
target_link_libraries(run_hello PRIVATE arviss)
target_compile_options(run_hello PRIVATE
        $<$<OR:$<C_COMPILER_ID:Clang>,$<C_COMPILER_ID:AppleClang>,$<C_COMPILER_ID:GNU>>:
        -Wall -Werror>
        $<$<C_COMPILER_ID:MSVC>:
        /WX>)

# Installation.
# TODO: fix it so that it doesn't install gtest.

install(TARGETS arviss
        EXPORT arvissTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
        )

install(FILES arviss.h memory.h result.h DESTINATION include/arviss)

install(EXPORT arvissTargets
        FILE arvissTargets.cmake
        NAMESPACE arviss::
        DESTINATION lib/cmake/arviss
        )
