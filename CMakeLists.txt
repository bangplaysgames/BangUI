cmake_minimum_required(VERSION 3.16)
project(BangUI)
set(CMAKE_CXX_STANDARD 17)

# Enable vcpkg toolchain if available
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
endif()


# Add vcpkg installed directory to CMAKE_PREFIX_PATH for package discovery
set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows" ${CMAKE_PREFIX_PATH})

# Add SDL3 include directory globally for all targets
include_directories("${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/include")

# Option to build dependencies only in a special configuration
option(BUILD_DEPENDENCIES "Build all dependencies" OFF)

# Source directories
add_subdirectory(src)
option(BUILD_TESTS "Build tests" OFF)
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Only add DiligentEngine if BUILD_DEPENDENCIES is ON
if(BUILD_DEPENDENCIES)
    add_subdirectory(DiligentEngine)
endif()
