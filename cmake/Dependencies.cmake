include_guard(GLOBAL)
include(FetchContent)

# esphome-api-client dependencies, all wired through FetchContent. Where a system
# package is available it is preferred (FIND_PACKAGE_ARGS) so that install/export
# stays available and clean-build time stays low.
#
#   asio      -> header-only; hand-made INTERFACE target esphome_api_asio
#   gtest     -> GTest::gtest_main (gated by ESPHOME_API_BUILD_TESTS)
#
# The proto layer (cmake/ProtoGen.cmake) and the Noise crypto (src/crypto/detail/,
# wired in CMakeLists.txt) are both in-tree, so the only thing fetched for the
# library itself is Asio.

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
    # BUILD_INTERFACE-wrapped: Asio is a PRIVATE, build-only (header-only) dep, so
    # the path must not leak into the installed export.
    target_include_directories(esphome_api_asio SYSTEM INTERFACE
        "$<BUILD_INTERFACE:${asio_SOURCE_DIR}/asio/include>")
    target_compile_definitions(esphome_api_asio INTERFACE
        ASIO_STANDALONE
        ASIO_NO_DEPRECATED)
    target_link_libraries(esphome_api_asio INTERFACE Threads::Threads)
endif()

# ---------------------------------------------------------------------------
# 2. GoogleTest — tests only.
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
# 3. esphome-cli tool dependencies — CLI11 (arg parsing), spdlog (the CLI's own
#    logging) and nlohmann_json (JSON). These are PRIVATE to the bin/ target and
#    never reach the library, so they do not affect ESPHOME_API_INSTALL.
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
