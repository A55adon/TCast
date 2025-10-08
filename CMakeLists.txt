cmake_minimum_required(VERSION 3.20)
project(TCast)

set(CMAKE_CXX_STANDARD 17)

set(SOURCES
        src/main.cpp
        src/Projector.cpp
        src/Window.cpp
        libs/RmlUi/Backends/RmlUi_Platform_GLFW.cpp
        libs/RmlUi/Backends/RmlUi_Renderer_GL3.cpp
        libs/RmlUi/Backends/RmlUi_Backend_GLFW_GL3.cpp
        libs/RmlUi/Samples/shell/src/PlatformExtensions.cpp
        libs/RmlUi/Samples/shell/src/RendererExtensions.cpp
        libs/RmlUi/Samples/shell/src/Shell.cpp
        libs/RmlUi/Samples/shell/src/ShellFileInterface.cpp
        include/Helper.h
)

add_executable(TCast ${SOURCES})

target_include_directories(TCast PRIVATE
        "${CMAKE_SOURCE_DIR}/include"
        "${CMAKE_SOURCE_DIR}/include/stb"
        "${CMAKE_SOURCE_DIR}/libs/RmlUi/include"
        "${CMAKE_SOURCE_DIR}/libs/GLFW/include"
        "${CMAKE_SOURCE_DIR}/libs/glew-2.1.0/include"
        "${CMAKE_SOURCE_DIR}/libs/RmlUi/Backends"
        "${CMAKE_SOURCE_DIR}/libs/RmlUi/Samples/shell/include"
        "${CMAKE_SOURCE_DIR}/libs/glad/include"
)

link_directories(
        "${CMAKE_SOURCE_DIR}/libs/RmlUi/build-mingw"
        "${CMAKE_SOURCE_DIR}/libs/GLFW/lib"
        "${CMAKE_SOURCE_DIR}/libs/glew-2.1.0/lib/Release/x64"
)

target_link_libraries(TCast
        "${CMAKE_SOURCE_DIR}/libs/RmlUi/build-mingw/librmlui.dll.a"
        "${CMAKE_SOURCE_DIR}/libs/RmlUi/build-mingw/librmlui_debugger.dll.a"
        "${CMAKE_SOURCE_DIR}/libs/GLFW/lib/libglfw3.dll.a"
        "${CMAKE_SOURCE_DIR}/libs/glew-2.1.0/lib/Release/x64/glew32.lib"
        opengl32
        Shlwapi
)

# Copy DLLs when building TCast
add_custom_command(TARGET TCast POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/libs/RmlUi/build-mingw/librmlui.dll"
        "${CMAKE_SOURCE_DIR}/libs/RmlUi/build-mingw/librmlui_debugger.dll"
        "${CMAKE_SOURCE_DIR}/libs/GLFW/lib/glfw3.dll"
        "${CMAKE_SOURCE_DIR}/libs/freetype_x64-windows/bin/freetype.dll"
        "${CMAKE_SOURCE_DIR}/libs/glew-2.1.0/bin/Release/Win32/glew32.dll"
        "${CMAKE_SOURCE_DIR}/libs/default/libstdc++-6.dll"
        "${CMAKE_SOURCE_DIR}/libs/default/libgcc_s_seh-1.dll"
        "${CMAKE_SOURCE_DIR}/libs/default/libwinpthread-1.dll"
        $<TARGET_FILE_DIR:TCast>
        COMMENT "Copying DLLs..."
)

# Always copy assets on build
#file(GLOB_RECURSE ASSET_FILES "${CMAKE_SOURCE_DIR}/assets/*")
add_custom_target(copy_assets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:TCast>/assets"
        DEPENDS ${ASSET_FILES}
        COMMENT "Copying assets..."
)

add_dependencies(TCast copy_assets)
