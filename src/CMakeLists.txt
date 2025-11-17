file(GLOB SRC_LIB "lib/*.cpp" "lib/*.h" "impl/*.cpp" "impl/*.h")
file(GLOB SRC_MODELS "models/*.cpp" "models/*.h")
file(GLOB SRC_STANDALONE "standalone/*.cpp" "standalone/*.h")

add_library(BangUI_lib ${SRC_LIB} ${SRC_MODELS} ${SRC_STANDALONE})
target_include_directories(BangUI_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/models
    ${CMAKE_CURRENT_SOURCE_DIR}/impl
    ${CMAKE_CURRENT_SOURCE_DIR}/standalone
    ${CMAKE_CURRENT_SOURCE_DIR}/standalone/thirdparty)

file(GLOB TEST_MAIN "models/test_main.cpp")
add_executable(BangUI_test_main ${TEST_MAIN})
target_link_libraries(BangUI_test_main PRIVATE BangUI_lib)

file(GLOB NATIVE_DEMO_SRC "lib/native_demo.cpp")
add_executable(BangUI_native_demo ${NATIVE_DEMO_SRC})
target_link_libraries(BangUI_native_demo PRIVATE BangUI_lib)
