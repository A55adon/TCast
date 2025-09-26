cmake_minimum_required(VERSION 3.20)
project(TCast)

set(CMAKE_CXX_STANDARD 17)



set(SOURCES
        src/main.cpp
        ../libs/RmlUi/Backends/RmlUi_Platform_GLFW.cpp
        ../libs/RmlUi/Backends/RmlUi_Renderer_GL3.cpp
        ../libs/RmlUi/Backends/RmlUi_Backend_GLFW_GL3.cpp
        ../libs/RmlUi/Samples/shell/src/PlatformExtensions.cpp
        ../libs/RmlUi/Samples/shell/src/RendererExtensions.cpp
        ../libs/RmlUi/Samples/shell/src/Shell.cpp
        ../libs/RmlUi/Samples/shell/src/ShellFileInterface.cpp
)

add_executable(TCast ${SOURCES})

# Include directories
target_include_directories(TCast PRIVATE
        "${CMAKE_SOURCE_DIR}/libs/RmlUi/include"
        "${CMAKE_SOURCE_DIR}/libs/GLFW/include"
        "${CMAKE_SOURCE_DIR}/libs/RmlUi/Backends"
        "${CMAKE_SOURCE_DIR}/libs/glew-2.1.0/include"  # for GLEW
        "${CMAKE_SOURCE_DIR}/include"
        "${CMAKE_SOURCE_DIR}/libs/RmlUi/Backends"
)

# Library directories
link_directories(
        "${CMAKE_SOURCE_DIR}/libs/RmlUi/build-mingw"
        "${CMAKE_SOURCE_DIR}/libs/GLFW/lib"
        "${CMAKE_SOURCE_DIR}/libs/glew-2.1.0/lib/Release/x64"      # for GLEW
)

# Link against RmlUi, GLFW, GLEW, and OpenGL
target_link_libraries(TCast
        "${CMAKE_SOURCE_DIR}/libs/RmlUi/build-mingw/librmlui.dll.a"
        "${CMAKE_SOURCE_DIR}/libs/RmlUi/build-mingw/librmlui_debugger.dll.a"
        "${CMAKE_SOURCE_DIR}/libs/GLFW/lib/libglfw3.dll.a"
        "${CMAKE_SOURCE_DIR}/libs/glew-2.1.0/lib/Release/x64/glew32.lib"
        opengl32
        Shlwapi
)


# Copy DLLs after build
add_custom_command(TARGET TCast POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/libs/RmlUi/build-mingw/librmlui.dll"
        "${CMAKE_SOURCE_DIR}/libs/RmlUi/build-mingw/librmlui_debugger.dll"
        "${CMAKE_SOURCE_DIR}/libs/GLFW/lib/glfw3.dll"
        "C:/msys64/mingw64/bin/glew32.dll"
        $<TARGET_FILE_DIR:TCast>
)
