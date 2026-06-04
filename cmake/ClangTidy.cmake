include_guard(GLOBAL)

# esphome_api_enable_clang_tidy(<target>)
#
# Applies clang-tidy to <target> when ESPHOME_API_ENABLE_CLANG_TIDY is ON.

if(ESPHOME_API_ENABLE_CLANG_TIDY)
    find_program(ESPHOME_API_CLANG_TIDY_EXE NAMES clang-tidy REQUIRED)
    set(ESPHOME_API_CLANG_TIDY_COMMAND
        "${ESPHOME_API_CLANG_TIDY_EXE}"
        "--config-file=${CMAKE_SOURCE_DIR}/.clang-tidy"
    )
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        execute_process(
            COMMAND "${CMAKE_CXX_COMPILER}" -print-search-dirs
            OUTPUT_VARIABLE _esphome_api_gcc_search_dirs
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(_esphome_api_gcc_search_dirs MATCHES "install: ([^\n]+)")
            list(APPEND ESPHOME_API_CLANG_TIDY_COMMAND
                "--extra-arg-before=--gcc-install-dir=${CMAKE_MATCH_1}")
            message(STATUS
                "esphome-api-client: pinning clang-tidy to gcc install dir ${CMAKE_MATCH_1}")
        endif()
    endif()
    message(STATUS "esphome-api-client: clang-tidy enabled (${ESPHOME_API_CLANG_TIDY_EXE})")
endif()

function(esphome_api_enable_clang_tidy target)
    if(NOT ESPHOME_API_ENABLE_CLANG_TIDY)
        return()
    endif()
    set_target_properties(${target} PROPERTIES
        CXX_CLANG_TIDY "${ESPHOME_API_CLANG_TIDY_COMMAND}"
    )
endfunction()
