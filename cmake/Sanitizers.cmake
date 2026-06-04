include_guard(GLOBAL)

set(ESPHOME_API_SANITIZER_FLAGS
    -fsanitize=address
    -fsanitize=undefined
    -fno-omit-frame-pointer
    -fno-sanitize-recover=all
)

# Apply sanitizer flags at directory scope so they propagate to every target
# declared after this module is included — including FetchContent-built
# dependencies. Mixed instrumentation otherwise causes libc++ container
# annotations to get out of sync between our code and uninstrumented deps,
# producing spurious container-overflow reports.
if(ESPHOME_API_ENABLE_SANITIZERS AND NOT MSVC)
    add_compile_options(${ESPHOME_API_SANITIZER_FLAGS})
    add_link_options   (${ESPHOME_API_SANITIZER_FLAGS})
elseif(ESPHOME_API_ENABLE_SANITIZERS AND MSVC)
    message(STATUS "esphome-api-client: sanitizers requested but skipped on MSVC")
endif()

# esphome_api_enable_sanitizers(<target>)
#
# Adds AddressSanitizer + UndefinedBehaviorSanitizer flags to <target> when
# ESPHOME_API_ENABLE_SANITIZERS is ON and the toolchain is GCC or Clang. The
# directory-scope add_compile_options above already covers all targets in this
# project; this function is kept for backward compatibility and as a no-op
# safety net for targets declared in unusual scopes.
function(esphome_api_enable_sanitizers target)
    if(NOT ESPHOME_API_ENABLE_SANITIZERS)
        return()
    endif()
    if(MSVC)
        return()
    endif()

    target_compile_options(${target} PRIVATE ${ESPHOME_API_SANITIZER_FLAGS})
    target_link_options   (${target} PRIVATE ${ESPHOME_API_SANITIZER_FLAGS})
endfunction()
