cmake_minimum_required(VERSION 3.21)
project(TCast)
set(BUILD_SHARED_LIBS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()



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
        src/Utilities.cpp
        src/UISetup.cpp
        src/UIManager.cpp
        src/EventListener.cpp
        src/nanosvg_impl.cpp
        src/ResourceHandler.cpp

        libs/imgui/imgui.cpp
        libs/imgui/imgui_draw.cpp
        libs/imgui/imgui_tables.cpp
        libs/imgui/imgui_widgets.cpp
        libs/imgui/backends/imgui_impl_glfw.cpp
        libs/imgui/backends/imgui_impl_opengl3.cpp
)

add_executable(TCast ${SOURCES})

target_compile_options(TCast PRIVATE
        $<$<CONFIG:Release>:-O3 -march=native -DNDEBUG>
)

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported)

if(ipo_supported)
    set_property(TARGET TCast PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
endif()


target_include_directories(TCast PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include/stb
        ${CMAKE_SOURCE_DIR}/libs/RmlUi/include
        ${CMAKE_SOURCE_DIR}/libs/GLFW/include
        ${CMAKE_SOURCE_DIR}/libs/glew-2.1.0/include
        ${CMAKE_SOURCE_DIR}/libs/RmlUi/Backends
        ${CMAKE_SOURCE_DIR}/libs/RmlUi/Samples/shell/include
        ${CMAKE_SOURCE_DIR}/libs/glad/include
        C:/msys64/mingw64/include
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include/imgui
        ${CMAKE_SOURCE_DIR}/include/imgui/backends

)

link_directories(
        "${CMAKE_SOURCE_DIR}/libs/RmlUi/build-mingw"
        "${CMAKE_SOURCE_DIR}/libs/GLFW/lib"
        "${CMAKE_SOURCE_DIR}/libs/glew-2.1.0/lib/Release/x64"
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(AVFORMAT REQUIRED libavformat)
pkg_check_modules(AVCODEC REQUIRED libavcodec)
pkg_check_modules(AVUTIL REQUIRED libavutil)
pkg_check_modules(SWSCALE REQUIRED libswscale)

list(REMOVE_ITEM AVFORMAT_LIBRARIES dl)
list(REMOVE_ITEM AVCODEC_LIBRARIES dl)
list(REMOVE_ITEM AVUTIL_LIBRARIES dl)
list(REMOVE_ITEM SWSCALE_LIBRARIES dl)

target_link_libraries(TCast
        ${CMAKE_SOURCE_DIR}/libs/RmlUi/build-mingw/librmlui.dll.a
        $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/libs/RmlUi/build-mingw/librmlui_debugger.dll.a>
        "${CMAKE_SOURCE_DIR}/libs/GLFW/lib/libglfw3.dll.a"
        "${CMAKE_SOURCE_DIR}/libs/glew-2.1.0/lib/Release/x64/glew32.lib"
        opengl32
        Shlwapi
        ${AVFORMAT_LIBRARIES}
        ${AVCODEC_LIBRARIES}
        ${AVUTIL_LIBRARIES}
        ${SWSCALE_LIBRARIES}
)


target_link_options(TCast PRIVATE
        $<$<CONFIG:Release>:-s>
)

set(MINGW_DLL_NAMES
        libgcc_s_seh-1
        libstdc++-6
        libwinpthread-1
)

set(MINGW_DLLS "")
foreach(DLL_NAME ${MINGW_DLL_NAMES})
    find_file(MINGW_DLL_${DLL_NAME}
            NAMES "${DLL_NAME}.dll"
            PATHS
            "C:/msys64/mingw64/bin"
            "C:/msys2/mingw64/bin"
            NO_DEFAULT_PATH
    )
    if(MINGW_DLL_${DLL_NAME})
        list(APPEND MINGW_DLLS "${MINGW_DLL_${DLL_NAME}}")
    else()
        message(WARNING "Could not find MinGW DLL: ${DLL_NAME}.dll")
    endif()
endforeach()

message(STATUS "MinGW DLLs to copy: ${MINGW_DLLS}")

set(FFMPEG_DLL_NAMES
        avutil-60
        avcodec-62
        avformat-62
        swscale-9
        swresample-6
)

# Automatically find all required DLLs using objdump
find_program(OBJDUMP objdump HINTS "C:/msys64/mingw64/bin" "C:/msys2/mingw64/bin")

if(OBJDUMP)
    add_custom_command(TARGET TCast POST_BUILD
            COMMAND ${CMAKE_COMMAND}
            -DOBJDUMP=${OBJDUMP}
            -DTARGET_EXE=$<TARGET_FILE:TCast>
            -DSEARCH_PATH=C:/msys64/mingw64/bin
            -DOUT_DIR=$<TARGET_FILE_DIR:TCast>
            -P "${CMAKE_SOURCE_DIR}/cmake/CopyDependencyDLLs.cmake"
            COMMENT "Copying all dependency DLLs..."
    )
else()
    message(WARNING "objdump not found, DLLs won't be copied automatically")
endif()

# Copy DLLs when building TCast
add_custom_command(TARGET TCast POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/libs/RmlUi/build-mingw/librmlui.dll"
        "${CMAKE_SOURCE_DIR}/libs/RmlUi/build-mingw/librmlui_debugger.dll"
        "${CMAKE_SOURCE_DIR}/libs/GLFW/lib/glfw3.dll"
        "${CMAKE_SOURCE_DIR}/libs/freetype_x64-windows/libfreetype-6.dll"
        "${CMAKE_SOURCE_DIR}/libs/glew-2.1.0/bin/Release/Win32/glew32.dll"
        ${MINGW_DLLS}
        $<TARGET_FILE_DIR:TCast>
        COMMENT "Copying DLLs..."
)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

file(GLOB_RECURSE ASSET_FILES
        ${CMAKE_SOURCE_DIR}/assets/*
)

add_custom_target(process_assets ALL
        COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/src/rcss_preprocessor.py
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:TCast>/assets
        DEPENDS ${ASSET_FILES}
        COMMENT "Processing assets (RCSS preprocessing + copy)"
)

add_custom_command(TARGET TCast POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Copying runtime DLLs"
        COMMAND ${CMAKE_COMMAND}
        -DDLLS="$<JOIN:$<TARGET_RUNTIME_DLLS:TCast>,|>"
        -DOUT_DIR="$<TARGET_FILE_DIR:TCast>"
        -P "${CMAKE_SOURCE_DIR}/cmake/CopyRuntimeDLLs.cmake"
        COMMAND_EXPAND_LISTS
)

target_compile_definitions(TCast PRIVATE VERSION=\"1.1.1\")


add_dependencies(TCast process_assets)
