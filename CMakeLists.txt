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
add_library(arviss STATIC arviss.c loadelf.c loadelf.h rounding_modes.h opcodes.h arviss_impl.h)
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

# Set _CRT_SECURE_NO_WARNINGS on MSVC or Clang on Windows so that we can use fopen(). Ideally we'd use fopen_s() as it's
# supposedly C11, but oddly the Linux compilers object to this.
target_compile_definitions(arviss PRIVATE
        $<$<OR:$<C_COMPILER_ID:MSVC>,$<AND:$<PLATFORM_ID:Windows>,$<C_COMPILER_ID:Clang>>>:_CRT_SECURE_NO_WARNINGS>
        )

# Arviss tests.
add_subdirectory("tests")

# Examples.
add_subdirectory("examples")

# Installation.
# TODO: fix it so that it doesn't install gtest.

install(TARGETS arviss
        EXPORT arvissTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
        )

install(FILES arviss.h loadelf.h memory.h result.h DESTINATION include/arviss)

install(EXPORT arvissTargets
        FILE arvissTargets.cmake
        NAMESPACE arviss::
        DESTINATION lib/cmake/arviss
        )
