include(FetchContent)

# Search for a raylib-config.cmake. Fetch it and build it if we can't find one.
find_package(raylib 3.7 QUIET)
if (NOT raylib_FOUND)

    FetchContent_Declare(
            raylib
            URL https://github.com/raysan5/raylib/archive/refs/tags/3.7.0.zip
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


# Run Very Angry Robots III.
add_executable(run_va_robots main.c game_loop.h screens.h screens.c menu.c playing.c contoller.h controller.c entities.c entities.h dynamic_components.c dynamic_components.h movement_system.c movement_system.h player_action_system.c player_action_system.h robot_action_system.c robot_action_system.h static_components.c static_components.h wall_components.c wall_components.h door_components.c door_components.h)
target_link_libraries(run_va_robots PRIVATE arviss raylib)
