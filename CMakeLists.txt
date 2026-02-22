cmake_minimum_required(VERSION 3.16)
project(falling_sand C)

# set the output directory for built objects
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# Use pkg-config to find SDL3
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL3 REQUIRED sdl3)

# Include SDL3 headers
include_directories(${SDL3_INCLUDE_DIRS})
link_directories(${SDL3_LIBRARY_DIRS})

# Create your executable
add_executable(falling_sand WIN32 
              src/main.c 
              src/grid/grid.c
              src/display/display.c
              src/particle/particle.c)

# Link SDL3 libraries
target_link_libraries(falling_sand PRIVATE ${SDL3_LIBRARIES})

# Tests
enable_testing()

add_executable(display_tests tests/test_display.c)
target_include_directories(display_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/display)
add_test(NAME display_tests COMMAND display_tests)

add_executable(grid_tests tests/test_grid.c)
target_include_directories(grid_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(grid_tests PRIVATE ${SDL3_LIBRARIES})
add_test(NAME grid_tests COMMAND grid_tests)

add_executable(particle_tests tests/test_particle.c)
target_include_directories(particle_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(particle_tests PRIVATE ${SDL3_LIBRARIES})
add_test(NAME particle_tests COMMAND particle_tests)
