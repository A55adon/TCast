# cmake/CopyRuntimeDLLs.cmake
if(NOT DLLS OR DLLS STREQUAL "")
    message(STATUS "No runtime DLLs to copy.")
    return()
endif()

string(REPLACE "|" ";" DLL_LIST "${DLLS}")

foreach(DLL IN LISTS DLL_LIST)
    if(EXISTS "${DLL}")
        message(STATUS "Copying runtime DLL: ${DLL}")
        file(COPY_FILE "${DLL}" "${OUT_DIR}/${DLL}" ONLY_IF_DIFFERENT)
    endif()
endforeach()