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

if (UNIX)
    set(THE_LINKER lld-11)
else ()
    set(THE_LINKER lld)
endif ()

set(CMAKE_C_LINK_FLAGS "-Wl,--gc-sections -Wl,-T,,${CMAKE_SOURCE_DIR}/minimal.ld -fuse-ld=${THE_LINKER} -nostartfiles")

project(hello
        LANGUAGES C ASM)

# Create the bin directory when CMake generates the build system.
set(SAMPLES_BIN "${CMAKE_SOURCE_DIR}/bin")
file(MAKE_DIRECTORY ${SAMPLES_BIN})

add_executable(hello crt0.s main.c printf.c putchar.c)
target_compile_definitions(hello PRIVATE PRINTF_DISABLE_SUPPORT_FLOAT PRINTF_DISABLE_SUPPORT_LONG_LONG)

add_custom_command(TARGET hello
        POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:hello> ${SAMPLES_BIN}/hello.bin
        VERBATIM
        )
