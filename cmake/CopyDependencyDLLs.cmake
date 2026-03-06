# Recursively find and copy all DLL dependencies
function(collect_deps exe search_path collected_var)
    execute_process(
            COMMAND ${OBJDUMP} -p "${exe}"
            OUTPUT_VARIABLE dump_output
    )

    string(REGEX MATCHALL "DLL Name: ([^\n]+\\.dll)" matches "${dump_output}")

    foreach(match ${matches})
        string(REGEX REPLACE "DLL Name: " "" dll_name "${match}")
        string(STRIP "${dll_name}" dll_name)

        if(dll_name IN_LIST ${collected_var})
            continue()
        endif()

        set(dll_path "${search_path}/${dll_name}")
        if(EXISTS "${dll_path}")
            list(APPEND ${collected_var} "${dll_name}")
            set(${collected_var} ${${collected_var}} PARENT_SCOPE)
            # Recurse into this DLL's dependencies too
            collect_deps("${dll_path}" "${search_path}" ${collected_var})
            set(${collected_var} ${${collected_var}} PARENT_SCOPE)
        endif()
    endforeach()
endfunction()

set(collected "")
collect_deps("${TARGET_EXE}" "${SEARCH_PATH}" collected)

foreach(dll_name ${collected})
    set(src "${SEARCH_PATH}/${dll_name}")
    message(STATUS "Copying: ${dll_name}")
    file(COPY_FILE "${src}" "${OUT_DIR}/${dll_name}" ONLY_IF_DIFFERENT)
endforeach()