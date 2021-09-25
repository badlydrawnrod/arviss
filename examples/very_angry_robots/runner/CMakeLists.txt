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
add_executable(run_va_robots main.c game_loop.h screens.h screens.c menu.c playing.c controller.h controller.c entities.c entities.h tables/positions.c tables/positions.h systems/movement_system.c systems/movement_system.h systems/player_action_system.c systems/player_action_system.h systems/robot_action_system.c systems/robot_action_system.h tables/velocities.c tables/velocities.h tables/walls.c tables/walls.h tables/doors.c tables/doors.h systems/drawing_system.c systems/drawing_system.h tables/collidables.c tables/collidables.h systems/collision_system.c systems/collision_system.h tables/events.c tables/events.h systems/event_system.c systems/event_system.h systems/reaper_system.c systems/reaper_system.h systems/game_status_system.c systems/game_status_system.h systems/collision_response_system.c systems/collision_response_system.h tables/player_status.c tables/player_status.h timed_triggers.h types.h tables/rooms.c tables/rooms.h tables/aims.c tables/aims.h systems/player_controller_system.c systems/player_controller_system.h tables/player_controls.c tables/player_controls.h)
target_include_directories(run_va_robots PRIVATE .)
target_link_libraries(run_va_robots PRIVATE arviss raylib)
