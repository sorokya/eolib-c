cmake_minimum_required(VERSION 2.8.2)
project(eolib_win32_demo C)

# Tell CMake where headers are
include_directories(${CMAKE_SOURCE_DIR}/include)

# Build your executable
add_executable(eolib_win32_demo main.c)

# Link the static import library
target_link_libraries(eolib_win32_demo ${CMAKE_SOURCE_DIR}/lib/libeolib.dll.a)