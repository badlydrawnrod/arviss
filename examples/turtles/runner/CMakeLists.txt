include(FetchContent)

# Search for a raylib-config.cmake. Fetch it and build it if we can't find one.
find_package(raylib 3.7 QUIET)
if (NOT raylib_FOUND)

    # Option 1 - get raylib from the latest master build.
    #
    FetchContent_Declare(
            raylib
            URL https://github.com/raysan5/raylib/archive/master.tar.gz
            #            URL https://github.com/raysan5/raylib/archive/refs/heads/master.zip
            #            If master is broken for some reason then uncomment this.
            #            GIT_REPOSITORY ${CMAKE_SOURCE_DIR}/../3rdparty/raylib
            #            GIT_TAG rod2
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
add_executable(run_turtles run_turtles.c)
target_link_libraries(run_turtles PRIVATE arviss raylib)
