cmake_minimum_required(VERSION 3.16)
project(BangUI)
set(CMAKE_CXX_STANDARD 17)

# Enable vcpkg toolchain if available
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
endif()


# Source directories
add_subdirectory(src)
option(BUILD_TESTS "Build tests" OFF)
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Configure Visual Studio F5/Debug to launch the native demo by default.
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT BangUI_native_demo)
