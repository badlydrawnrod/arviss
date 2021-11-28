cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

set(CMAKE_CROSSCOMPILING 1)

set(CMAKE_SYSTEM_NAME Generic)

# Silence CMake's compiler checks because they'll fail.
set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)

set(CMAKE_EXE_LINKER_FLAGS_INIT "")
set(CMAKE_EXE_LINKER_FLAGS "")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "")

set(CMAKE_C_FLAGS_INIT "")
set(CMAKE_C_FLAGS "--target=riscv32 -ffreestanding -nostdlib -nodefaultlibs -march=rv32imf -mabi=ilp32f -ffunction-sections -fdata-sections")
set(CMAKE_ASM_FLAGS "--target=riscv32 -ffreestanding -nostdlib -nodefaultlibs -march=rv32imf -mabi=ilp32f -ffunction-sections -fdata-sections")

# Allow the linker to be overridden on the cmake command line, e.g., -DLINKER=lld-12
if (NOT DEFINED ARVISS_LINKER)
    set(ARVISS_LINKER lld)
endif ()

set(CMAKE_C_LINK_FLAGS "-Wl,--gc-sections -Wl,-T,,${CMAKE_SOURCE_DIR}/minimal.ld -Wl,--nmagic -fuse-ld=${ARVISS_LINKER} -nostartfiles")

project(cell
        LANGUAGES C ASM)

# Create the bin directory when CMake generates the build system.
set(SAMPLES_BIN "${CMAKE_SOURCE_DIR}/bin")
file(MAKE_DIRECTORY ${SAMPLES_BIN})

add_executable(cell crt0.s main.c cell.h syscalls.h)

# Copy "cell" to the bin directory once built.
add_custom_command(TARGET cell
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:cell> ${SAMPLES_BIN}/cell
        )
