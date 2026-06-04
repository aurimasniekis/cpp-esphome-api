include_guard(GLOBAL)
include(FetchContent)

# esphome-api-client dependencies, all wired through FetchContent. Where a system
# package is available it is preferred (FIND_PACKAGE_ARGS) so that install/export
# can be re-enabled and clean-build time stays low.
#
#   asio      -> header-only; hand-made INTERFACE target esphome_api_asio
#   libsodium -> target `sodium` (gated by ESPHOME_API_WITH_NOISE)
#   gtest     -> GTest::gtest_main (gated by ESPHOME_API_BUILD_TESTS)
#
# The proto layer is self-contained (see cmake/ProtoGen.cmake): a build-time C++
# generator emits the message classes + a static id<->type table, so there is no
# protobuf / abseil dependency to fetch or link.

# ---------------------------------------------------------------------------
# 1. Asio (standalone). No CMake package — build a hand-made INTERFACE target.
#    Kept PRIVATE to the library so consumers never need ASIO_STANDALONE.
# ---------------------------------------------------------------------------
FetchContent_Declare(
    asio
    URL      https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-1-30-2.tar.gz
    URL_HASH SHA256=755bd7f85a4b269c67ae0ea254907c078d408cce8e1a352ad2ed664d233780e8
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(asio)

if(NOT TARGET esphome_api_asio)
    add_library(esphome_api_asio INTERFACE)
    target_include_directories(esphome_api_asio SYSTEM INTERFACE
        "${asio_SOURCE_DIR}/asio/include")
    target_compile_definitions(esphome_api_asio INTERFACE
        ASIO_STANDALONE
        ASIO_NO_DEPRECATED)
    target_link_libraries(esphome_api_asio INTERFACE Threads::Threads)
endif()

# ---------------------------------------------------------------------------
# 3. libsodium (via robinlinden/libsodium-cmake) — Noise crypto primitives.
#    Provides target `sodium`. Only needed when Noise support is enabled.
# ---------------------------------------------------------------------------
if(ESPHOME_API_WITH_NOISE)
    set(SODIUM_DISABLE_TESTS ON CACHE INTERNAL "")
    set(SODIUM_MINIMAL       ON CACHE INTERNAL "")
    FetchContent_Declare(
        Sodium
        GIT_REPOSITORY https://github.com/robinlinden/libsodium-cmake.git
        GIT_TAG        9b2848dfc1b917a9410f0de9d81059b26cbfaa8d
    )
    FetchContent_MakeAvailable(Sodium)

    if(DEFINED sodium_SOURCE_DIR OR DEFINED Sodium_SOURCE_DIR)
        set(ESPHOME_API_SODIUM_FETCHED TRUE CACHE INTERNAL "")
    endif()

    # Normalise the target name to `esphome::sodium` for internal linking.
    if(TARGET sodium AND NOT TARGET esphome::sodium)
        add_library(esphome::sodium ALIAS sodium)
    endif()
endif()

# ---------------------------------------------------------------------------
# 4. GoogleTest — tests only.
# ---------------------------------------------------------------------------
if(ESPHOME_API_BUILD_TESTS)
    set(INSTALL_GTEST OFF CACHE INTERNAL "")
    set(BUILD_GMOCK   OFF CACHE INTERNAL "")
    FetchContent_Declare(
        googletest
        URL      https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz
        URL_HASH SHA256=65fab701d9829d38cb77c14acdc431d2108bfdbf8979e40eb8ae567edf10b27c
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        FIND_PACKAGE_ARGS NAMES GTest
    )
    FetchContent_MakeAvailable(googletest)
endif()

# ---------------------------------------------------------------------------
# 5. esphome-cli tool dependencies — CLI11 (arg parsing), spdlog (the CLI's own
#    logging) and nlohmann_json (JSON). These are PRIVATE to the bin/ target and
#    never reach the library, so they do not affect ESPHOME_API_INSTALL (which is
#    only disabled when protobuf/abseil/sodium are fetched from source).
# ---------------------------------------------------------------------------
if(ESPHOME_API_BUILD_CLI)
    set(CLI11_BUILD_TESTS    OFF CACHE INTERNAL "")
    set(CLI11_BUILD_EXAMPLES OFF CACHE INTERNAL "")
    set(CLI11_BUILD_DOCS     OFF CACHE INTERNAL "")
    set(CLI11_INSTALL        OFF CACHE INTERNAL "")
    FetchContent_Declare(
        CLI11
        URL      https://github.com/CLIUtils/CLI11/archive/refs/tags/v2.4.2.tar.gz
        URL_HASH SHA256=f2d893a65c3b1324c50d4e682c0cdc021dd0477ae2c048544f39eed6654b699a
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        FIND_PACKAGE_ARGS NAMES CLI11
    )

    set(SPDLOG_BUILD_TESTS   OFF CACHE INTERNAL "")
    set(SPDLOG_BUILD_EXAMPLE OFF CACHE INTERNAL "")
    set(SPDLOG_INSTALL       OFF CACHE INTERNAL "")
    FetchContent_Declare(
        spdlog
        URL      https://github.com/gabime/spdlog/archive/refs/tags/v1.14.1.tar.gz
        URL_HASH SHA256=1586508029a7d0670dfcb2d97575dcdc242d3868a259742b69f100801ab4e16b
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        FIND_PACKAGE_ARGS NAMES spdlog
    )

    set(JSON_BuildTests OFF CACHE INTERNAL "")
    set(JSON_Install    OFF CACHE INTERNAL "")
    FetchContent_Declare(
        nlohmann_json
        URL      https://github.com/nlohmann/json/archive/refs/tags/v3.11.3.tar.gz
        URL_HASH SHA256=0d8ef5af7f9794e3263480193c491549b2ba6cc74bb018906202ada498a79406
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        FIND_PACKAGE_ARGS NAMES nlohmann_json
    )

    FetchContent_MakeAvailable(CLI11 spdlog nlohmann_json)
endif()
