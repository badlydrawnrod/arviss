include(FetchContent)

# Search for a raylib-config.cmake. Fetch it and build it if we can't find one.
find_package(raylib 3.7 QUIET)
if (NOT raylib_FOUND)

    FetchContent_Declare(
            raylib
            URL https://github.com/raysan5/raylib/archive/master.tar.gz
    )

    FetchContent_GetProperties(raylib)

    # Download and build raylib.
    if (NOT raylib_POPULATED) # Have we downloaded raylib yet?
        set(FETCHCONTENT_QUIET NO)
        FetchContent_Populate(raylib)

        # Don't build the raylib examples.
        set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

        # Build raylib
        add_subdirectory(${raylib_SOURCE_DIR} ${raylib_BINARY_DIR})
    endif ()
endif ()


# Run the turtles on via Arviss.
add_executable(run_turtles run_turtles.c mem.c mem.h syscalls.h)
target_link_libraries(run_turtles PRIVATE arviss raylib)

# Set _CRT_SECURE_NO_WARNINGS on MSVC or Clang on Windows so that we can use fopen(). Ideally we'd use fopen_s() as it's
# supposedly C11, but oddly the Linux compilers object to this.
target_compile_definitions(run_turtles PRIVATE
        $<$<OR:$<C_COMPILER_ID:MSVC>,$<AND:$<PLATFORM_ID:Windows>,$<C_COMPILER_ID:Clang>>>:_CRT_SECURE_NO_WARNINGS>
        )
