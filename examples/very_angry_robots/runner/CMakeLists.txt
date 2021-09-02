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
add_executable(run_va_robots main.c game_loop.h screens.h screens.c menu.c playing.c contoller.h controller.c entities.c entities.h components/dynamic_components.c components/dynamic_components.h systems/movement_system.c systems/movement_system.h systems/player_action_system.c systems/player_action_system.h systems/robot_action_system.c systems/robot_action_system.h components/static_components.c components/static_components.h components/wall_components.c components/wall_components.h components/door_components.c components/door_components.h systems/drawing_system.c systems/drawing_system.h components/collidable_components.c components/collidable_components.h)
target_include_directories(run_va_robots PRIVATE .)
target_link_libraries(run_va_robots PRIVATE arviss raylib)
