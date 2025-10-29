# Set SDL3_DIR for CMake to find SDL3Config.cmake
set(SDL3_DIR "${CMAKE_SOURCE_DIR}/../vcpkg/installed/x64-windows/share/sdl3")
set(SDL3_image_DIR "${CMAKE_SOURCE_DIR}/../vcpkg/installed/x64-windows/share/sdl3-image")
set(SDL3_ttf_DIR "${CMAKE_SOURCE_DIR}/../vcpkg/installed/x64-windows/share/sdl3-ttf")

file(GLOB SRC_LIB "lib/*.cpp" "lib/*.h" "impl/*.cpp" "impl/*.h")
# Remove sdl_diligent_demo.cpp from SRC_LIB
list(REMOVE_ITEM SRC_LIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/sdl_diligent_demo.cpp")
file(GLOB SRC_MODELS "models/*.cpp" "models/*.h")
add_library(BangUI_lib ${SRC_LIB} ${SRC_MODELS})
target_include_directories(BangUI_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib ${CMAKE_CURRENT_SOURCE_DIR}/models ${CMAKE_CURRENT_SOURCE_DIR}/impl "${CMAKE_SOURCE_DIR}/../vcpkg/installed/x64-windows/include")

file(GLOB TEST_MAIN "models/test_main.cpp")
add_executable(BangUI_test_main ${TEST_MAIN})
target_link_libraries(BangUI_test_main PRIVATE BangUI_lib)



# Find SDL3 only (always required)
find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_image CONFIG REQUIRED)
find_package(SDL3_ttf CONFIG REQUIRED)


# Always build BangUI_sdl_demo for testing
find_package(DiligentEngine CONFIG QUIET)
file(GLOB SDL_DEMO_SRC "lib/sdl_diligent_demo.cpp")
add_executable(BangUI_sdl_demo ${SDL_DEMO_SRC})
if(DiligentEngine_FOUND)
	target_link_libraries(BangUI_sdl_demo PRIVATE BangUI_lib SDL3::SDL3 $<IF:$<TARGET_EXISTS:SDL3_image::SDL3_image-shared>,SDL3_image::SDL3_image-shared,SDL3_image::SDL3_image-static> SDL3_ttf::SDL3_ttf DiligentEngine::GraphicsEngineD3D11 DiligentEngine::GraphicsEngineD3D12 DiligentEngine::GraphicsEngineOpenGL DiligentEngine::GraphicsEngineVk)
else()
	target_link_libraries(BangUI_sdl_demo PRIVATE BangUI_lib SDL3::SDL3 $<IF:$<TARGET_EXISTS:SDL3_image::SDL3_image-shared>,SDL3_image::SDL3_image-shared,SDL3_image::SDL3_image-static> SDL3_ttf::SDL3_ttf)
endif()
target_include_directories(BangUI_sdl_demo PRIVATE "${CMAKE_SOURCE_DIR}/../vcpkg/installed/x64-windows/include")

# Copy vcpkg runtime DLLs next to executables so they can be loaded when running/debugging
if(EXISTS "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/bin")
    file(GLOB VCPKG_RUNTIME_DLLS "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/bin/*.dll")
    foreach(_dll IN LISTS VCPKG_RUNTIME_DLLS)
        # Copy DLLs to BangUI_sdl_demo output directory after build
        add_custom_command(TARGET BangUI_sdl_demo POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_dll}" "$<TARGET_FILE_DIR:BangUI_sdl_demo>"
            COMMENT "Copying runtime DLL ${_dll} to target output"
        )
        # Also copy to test main and tests executables if they exist
        add_custom_command(TARGET BangUI_test_main POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_dll}" "$<TARGET_FILE_DIR:BangUI_test_main>"
            COMMENT "Copying runtime DLL ${_dll} to test output"
        )
    endforeach()
endif()

# Copy SDL3 DLLs and dependencies to output directory after build
add_custom_command(TARGET BangUI_sdl_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/bin/SDL3.dll"
        "$<TARGET_FILE_DIR:BangUI_sdl_demo>/SDL3.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/bin/SDL3_image.dll"
        "$<TARGET_FILE_DIR:BangUI_sdl_demo>/SDL3_image.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/bin/SDL3_ttf.dll"
        "$<TARGET_FILE_DIR:BangUI_sdl_demo>/SDL3_ttf.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/bin/libpng16.dll"
        "$<TARGET_FILE_DIR:BangUI_sdl_demo>/libpng16.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/bin/zlib1.dll"
        "$<TARGET_FILE_DIR:BangUI_sdl_demo>/zlib1.dll"
    COMMENT "Copying SDL3 DLLs and image codec dependencies to output directory"
)

# Option: register the SDL demo as the single integration test harness.
# Note: the demo application is intentionally a manual, interactive test harness and is
# not registered as an automated CTest test. Human operators should run the demo to
# validate UI behavior and interactions.
