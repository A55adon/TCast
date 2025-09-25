cmake_minimum_required(VERSION 3.20)
project(TCast)

set(CMAKE_CXX_STANDARD 17)

set(SOURCES
        src/main.cpp
)

add_executable(TCast ${SOURCES})

set(RMLUI_DLL_DIR "${CMAKE_SOURCE_DIR}/libs/RmlUi/build")
set(RMLUI_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/libs/RmlUi/include")

target_include_directories(TCast PRIVATE
        "${RMLUI_INCLUDE_DIR}"
)
target_link_directories(TCast PRIVATE "${RMLUI_DLL_DIR}/Release")

add_custom_command(TARGET TCast POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${RMLUI_DLL_DIR}/Release/rmlui.dll"
        "${RMLUI_DLL_DIR}/Release/rmlui_debugger.dll"
        $<TARGET_FILE_DIR:TCast>
)
