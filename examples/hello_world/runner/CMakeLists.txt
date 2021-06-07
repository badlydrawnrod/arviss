
# Run "hello world" via Arviss.
add_executable(run_hello run_hello.c mem.c mem.h)

# Link with libm for platforms that need it.
target_link_libraries(run_hello PRIVATE arviss
        $<$<AND:$<NOT:$<PLATFORM_ID:Windows>>,$<OR:$<C_COMPILER_ID:Clang>,$<C_COMPILER_ID:AppleClang>,$<C_COMPILER_ID:GNU>>>:m>
        )

# Set _CRT_SECURE_NO_WARNINGS on MSVC or Clang on Windows so that we can use fopen(). Ideally we'd use fopen_s() as it's
# supposedly C11, but oddly the Linux compilers object to this.
target_compile_definitions(run_hello PRIVATE
        $<$<OR:$<C_COMPILER_ID:MSVC>,$<AND:$<PLATFORM_ID:Windows>,$<C_COMPILER_ID:Clang>>>:_CRT_SECURE_NO_WARNINGS>
        )

target_compile_options(run_hello PRIVATE
        $<$<OR:$<C_COMPILER_ID:Clang>,$<C_COMPILER_ID:AppleClang>,$<C_COMPILER_ID:GNU>>:
        -Wall -Werror>
        $<$<C_COMPILER_ID:MSVC>:
        /WX>)
