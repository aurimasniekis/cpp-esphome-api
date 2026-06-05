cmake_minimum_required(VERSION 3.25)

project(esphome-api-client
    VERSION     0.4.1
    DESCRIPTION "Modern C++17 client for the ESPHome native API (plaintext + Noise)"
    LANGUAGES   CXX
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        Debug Release RelWithDebInfo MinSizeRel)
endif()

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(ESPHOME_API_BUILD_TESTS      "Build tests"                       ${PROJECT_IS_TOP_LEVEL})
option(ESPHOME_API_BUILD_EXAMPLES   "Build examples"                    ${PROJECT_IS_TOP_LEVEL})
option(ESPHOME_API_BUILD_DOCS       "Build Doxygen API documentation"   OFF)
option(ESPHOME_API_INSTALL          "Generate install rules"           ${PROJECT_IS_TOP_LEVEL})
option(ESPHOME_API_ENABLE_CLANG_TIDY "Run clang-tidy during build"      OFF)
option(ESPHOME_API_ENABLE_SANITIZERS "Build with ASan + UBSan (Debug)"  OFF)
option(ESPHOME_API_ENABLE_COVERAGE  "Build with Clang source-based coverage" OFF)
option(ESPHOME_API_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ${PROJECT_IS_TOP_LEVEL})
option(ESPHOME_API_WITH_NOISE       "Enable Noise (encrypted) transport (self-contained in-tree crypto)" ON)
option(ESPHOME_API_BUILD_SHARED     "Build esphome-api-client as a shared library" OFF)
option(ESPHOME_API_BUILD_CLI        "Build the esphome-cli command-line tool" ${PROJECT_IS_TOP_LEVEL})
# Install rules for the esphome-cli executable. Decoupled from ESPHOME_API_INSTALL
# (which governs the library + headers + CMake package export) so the .deb/.rpm
# packaging can ship just the CLI binary without the library dev files.
option(ESPHOME_API_INSTALL_CLI      "Generate install rules for the esphome-cli executable" ${ESPHOME_API_INSTALL})

include(CompilerWarnings)
include(Sanitizers)
include(Coverage)
include(ClangTidy)
include(Dependencies)

find_package(Threads REQUIRED)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/esphome/api/version.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/include/esphome/api/version.hpp"
    @ONLY
)

# ---------------------------------------------------------------------------
# Proto pipeline (wired in M1): generates api.pb.{h,cc}, message_id.hpp and a
# proto object library into ${ESPHOME_API_GENERATED_DIR}.
# ---------------------------------------------------------------------------
set(ESPHOME_API_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ProtoGen.cmake")
    include(ProtoGen)
endif()

# ---------------------------------------------------------------------------
# Library
# ---------------------------------------------------------------------------
set(ESPHOME_API_SOURCES
    src/bytes.cpp
    src/proto/message_registry.cpp
    src/frame/plaintext_frame_helper.cpp
    src/transport/tcp_transport.cpp
    src/transport/asio_executor.cpp
    src/connection/connection.cpp
    src/model/device_info.cpp
    src/model/entity_type.cpp
    src/model/entity_store.cpp
    src/commands/command_builder.cpp
    src/discovery.cpp
    src/client.cpp
    src/sync_client.cpp
)

file(GLOB ESPHOME_API_ENTITY_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/model/entities/*.cpp")
list(APPEND ESPHOME_API_SOURCES ${ESPHOME_API_ENTITY_SOURCES})

file(GLOB ESPHOME_API_SUBSYSTEM_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/subsystems/*.cpp")
list(APPEND ESPHOME_API_SOURCES ${ESPHOME_API_SUBSYSTEM_SOURCES})

if(ESPHOME_API_WITH_NOISE)
    list(APPEND ESPHOME_API_SOURCES
        src/crypto/primitives.cpp
        src/crypto/cipher_state.cpp
        src/crypto/symmetric_state.cpp
        src/crypto/noise_handshake.cpp
        src/frame/noise_frame_helper.cpp
    )
endif()

if(ESPHOME_API_BUILD_SHARED)
    add_library(esphome-api-client SHARED ${ESPHOME_API_SOURCES})
else()
    add_library(esphome-api-client STATIC ${ESPHOME_API_SOURCES})
endif()
add_library(esphome::api ALIAS esphome-api-client)

target_include_directories(esphome-api-client
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${ESPHOME_API_GENERATED_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(esphome-api-client PUBLIC cxx_std_17)

target_link_libraries(esphome-api-client
    PUBLIC
        Threads::Threads
    PRIVATE
        esphome_api_asio
)

if(TARGET esphome_api_proto)
    target_link_libraries(esphome-api-client PUBLIC esphome_api_proto)
    # message_registry.cpp includes the generated message_id.hpp; make sure the
    # host tool has produced it before the library is compiled.
    add_dependencies(esphome-api-client esphome_api_message_id)
endif()

if(ESPHOME_API_WITH_NOISE)
    # PUBLIC so config.hpp::noise_supported and consumer code (examples, tests)
    # observe the same feature flag the library was built with.
    target_compile_definitions(esphome-api-client PUBLIC ESPHOME_API_HAS_NOISE=1)

    # Vendored, public-domain crypto primitives (SHA-256, ChaCha20-Poly1305,
    # X25519) + native CSPRNG. These are reference implementations that do NOT
    # satisfy our strict warning set / clang-tidy, so — mirroring the proto
    # object-library precedent (cmake/ProtoGen.cmake) — they are compiled in
    # their own static library with strict warnings and clang-tidy OFF, and
    # linked PRIVATE. The strict-warning wrapper (src/crypto/primitives.cpp)
    # stays in the main target and includes only the prototype headers.
    add_library(esphome_api_crypto STATIC
        src/crypto/detail/sha256.cpp
        src/crypto/detail/chacha20poly1305.cpp
        src/crypto/detail/x25519.cpp
        src/crypto/detail/random.cpp
    )
    target_compile_features(esphome_api_crypto PRIVATE cxx_std_17)
    target_include_directories(esphome_api_crypto PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include")
    set_target_properties(esphome_api_crypto PROPERTIES POSITION_INDEPENDENT_CODE ON)
    # random.cpp uses BCryptGenRandom on Windows.
    if(WIN32)
        target_link_libraries(esphome_api_crypto PRIVATE bcrypt)
    endif()

    target_link_libraries(esphome-api-client PRIVATE esphome_api_crypto)
endif()

esphome_api_set_warnings(esphome-api-client PRIVATE)
# GCC's -Wnull-dereference is an optimizer-pass warning, so at -O3 it fires from
# inside Asio's own headers (io_context.hpp / scheduler.ipp) once they are
# inlined into our transport TUs — the -isystem suppression is lost in the
# middle-end. The code is correct; downgrade this false positive from an error
# (same workaround the CLI uses for nlohmann/json). Must come after -Werror.
target_compile_options(esphome-api-client PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wno-error=null-dereference>)
esphome_api_enable_sanitizers(esphome-api-client)
esphome_api_enable_coverage(esphome-api-client)
esphome_api_enable_clang_tidy(esphome-api-client)

# ---------------------------------------------------------------------------
# Tests / examples / docs
# ---------------------------------------------------------------------------
if(ESPHOME_API_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(ESPHOME_API_BUILD_EXAMPLES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt")
    add_subdirectory(examples)
endif()

if(ESPHOME_API_BUILD_CLI AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bin/CMakeLists.txt")
    add_subdirectory(bin)
endif()

if(ESPHOME_API_BUILD_DOCS)
    find_package(Doxygen REQUIRED)
    set(ESPHOME_API_DOXYGEN_OUTPUT_DIR "${CMAKE_BINARY_DIR}/docs")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in"
        "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
        @ONLY
    )
    add_custom_target(esphome_api_docs
        COMMAND ${DOXYGEN_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Generating Doxygen documentation"
        VERBATIM
    )
endif()

# ---------------------------------------------------------------------------
# Install / export. The library and all of its build-time helpers are in-tree,
# so the only runtime dependency the installed package declares is Threads.
# ---------------------------------------------------------------------------
if(ESPHOME_API_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    # esphome-api-client (a static lib by default) links these in-tree helper
    # targets, so they are part of its link closure and must travel in the same
    # export set. Their interface include dirs are BUILD_INTERFACE-wrapped, so the
    # installed targets carry no build-tree paths; consumers get every public
    # header through the main target's INSTALL_INTERFACE include dir below.
    #   esphome_api_asio   — header-only INTERFACE (PRIVATE, build-only)
    #   esphome_api_proto  — generated message classes (PUBLIC)
    #   esphome_api_crypto — vendored Noise crypto (PRIVATE; Noise builds only)
    set(_esphome_api_export_aux esphome_api_asio)
    if(TARGET esphome_api_proto)
        list(APPEND _esphome_api_export_aux esphome_api_proto)
    endif()
    if(TARGET esphome_api_crypto)
        list(APPEND _esphome_api_export_aux esphome_api_crypto)
    endif()
    install(TARGETS ${_esphome_api_export_aux}
        EXPORT  esphome-api-clientTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

    install(TARGETS esphome-api-client
        EXPORT  esphome-api-clientTargets
        ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(DIRECTORY include/esphome
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.hpp"
    )
    install(DIRECTORY "${ESPHOME_API_GENERATED_DIR}/include/esphome"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.hpp"
    )

    install(EXPORT esphome-api-clientTargets
        FILE        esphome-api-clientTargets.cmake
        NAMESPACE   esphome::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/esphome-api-client
    )

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/esphome-api-clientConfigVersion.cmake"
        VERSION       ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )
    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/esphome-api-clientConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/esphome-api-clientConfig.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/esphome-api-client
    )
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/esphome-api-clientConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/esphome-api-clientConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/esphome-api-client
    )
endif()

# ---------------------------------------------------------------------------
# CPack — .deb / .rpm packaging for the CLI. Must come after all install()
# rules. Only the top-level project drives packaging.
# ---------------------------------------------------------------------------
if(PROJECT_IS_TOP_LEVEL)
    include(Packaging)
endif()
