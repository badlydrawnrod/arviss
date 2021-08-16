
# Run "hello world" via Arviss.
add_executable(run_hello run_hello.c mem.h)

# Link with libm for platforms that need it.
target_link_libraries(run_hello PRIVATE arviss
        $<$<AND:$<NOT:$<PLATFORM_ID:Windows>>,$<OR:$<C_COMPILER_ID:Clang>,$<C_COMPILER_ID:AppleClang>,$<C_COMPILER_ID:GNU>>>:m>
        )

target_compile_options(run_hello PRIVATE
        $<$<OR:$<C_COMPILER_ID:Clang>,$<C_COMPILER_ID:AppleClang>,$<C_COMPILER_ID:GNU>>:
        -Wall -Werror>
        $<$<C_COMPILER_ID:MSVC>:
        /WX>)
