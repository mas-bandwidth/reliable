cmake_minimum_required(VERSION 3.15)

# A no-op build prints nothing (rowan-new#110). The per-target "Built target" lines are
# the only output a rebuild with nothing to do produces, and they carry no information; a
# real build still prints its [ x%] progress lines, and errors are untouched. This sits
# above project() because CMAKE_TARGET_MESSAGES initializes the TARGET_MESSAGES global
# property there and has no effect if set after it, and under the same top-level test as the
# RELIABLE_TOP_LEVEL guard below, so a consumer that pulls this library in through FetchContent
# keeps its own messages.
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(CMAKE_TARGET_MESSAGES OFF)
endif()

# C only. the library is C99 and nothing in it needs a C++ compiler; the test driver is the
# one C++ translation unit in the tree, so CXX is enabled below only when tests are built.
# a consumer that has no C++ compiler configures and builds with -DRELIABLE_BUILD_TESTS=OFF
project(reliable VERSION 1.4.2 LANGUAGES C)

# is this the top level project, or pulled in via add_subdirectory / FetchContent?
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(RELIABLE_TOP_LEVEL TRUE)
else()
    set(RELIABLE_TOP_LEVEL FALSE)
endif()

option(RELIABLE_SANITIZE "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(RELIABLE_BUILD_TESTS "Build reliable tests, examples and harnesses" ${RELIABLE_TOP_LEVEL})

# test.cpp is the only C++ in the tree
if(RELIABLE_BUILD_TESTS)
    enable_language(CXX)
endif()

# default to debug, same as the old premake makefiles. only when this is the top level
# project: a consumer that pulls the library in through add_subdirectory or FetchContent
# picks its own build type, and forcing the cache variable would overwrite it
if(RELIABLE_TOP_LEVEL AND NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release)
endif()

# static MSVC runtime by default, matching the old premake configuration. package
# managers (e.g. vcpkg) pass their own CMAKE_MSVC_RUNTIME_LIBRARY to match the
# CRT linkage their consumers expect.
if(NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# strict floating point. Estate policy for mas-bandwidth network libraries: "generally
# speaking, network libraries we work on require -ffp-contract=off" (Glenn, 2026-08-24).
#   - GCC/Clang: -ffp-contract=off. Not merely "no -ffast-math" -- GCC's default is
#     -ffp-contract=fast, which contracts across statements, and clang's default =on still
#     fuses within a single expression.
#   - MSVC: /fp:precise, which since VS 2022 does not imply contraction.
# reliable's wire format carries no floating point -- the floats here are the RTT, jitter,
# packet loss and bandwidth stats -- so this is the family floor rather than a wire fix:
# contraction is architecture-dependent (FMA is in the aarch64 baseline and absent from
# the x86-64 one), so any float arithmetic that ever nears the wire diverges bit-wise
# between architectures without it. CI builds through this file, so every CI leg builds
# with these flags.
if(MSVC)
    add_compile_options(/W4 /fp:precise)
else()
    add_compile_options(-Wall -Wextra -ffp-contract=off)
endif()

if(RELIABLE_SANITIZE)
    if(MSVC)
        add_compile_options(/fsanitize=address)
    else()
        add_compile_options(-fsanitize=address,undefined -fno-sanitize-recover=all)
        add_link_options(-fsanitize=address,undefined)
    endif()
endif()

# the library. static by default; pass -DBUILD_SHARED_LIBS=ON for a shared library.
# the unit tests are NOT compiled into the library (the test binary compiles its own
# copy of reliable.c below), so this is what gets installed

add_library(reliable reliable.c reliable.h)

target_compile_features(reliable PUBLIC c_std_99)

target_include_directories(reliable PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>)

target_compile_definitions(reliable
    PUBLIC
        $<$<CONFIG:Debug>:RELIABLE_DEBUG>
        $<$<CONFIG:Release>:RELIABLE_RELEASE>
)

# the public surface is the RELIABLE_EXPORT declarations in reliable.h and nothing else.
# hidden visibility plus the export macro means a shared build exports exactly those, on every
# platform, rather than every global the compiler happens to emit
set_target_properties(reliable PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    C_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON)

if(BUILD_SHARED_LIBS)
    target_compile_definitions(reliable PUBLIC RELIABLE_SHARED)
    target_compile_definitions(reliable PRIVATE RELIABLE_BUILDING)
endif()

# reliable.c calls pow() for the jitter stddev, which needs libm where it is a separate
# library (Linux glibc; on mac and Windows the math functions live in the base runtime).
# PUBLIC so every executable linking the static library gets it on its link line.
find_library(RELIABLE_MATH_LIBRARY m)
if(RELIABLE_MATH_LIBRARY)
    target_link_libraries(reliable PUBLIC ${RELIABLE_MATH_LIBRARY})
endif()

if(RELIABLE_BUILD_TESTS)

    # "test" is a reserved target name in cmake, so the target is reliable_test
    # but the binary is still bin/test. it compiles reliable.c itself with the
    # embedded test suite enabled

    add_executable(reliable_test test.cpp reliable.c)
    set_target_properties(reliable_test PROPERTIES OUTPUT_NAME test)
    target_include_directories(reliable_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_definitions(reliable_test PRIVATE
        RELIABLE_ENABLE_TESTS=1
        $<$<CONFIG:Debug>:RELIABLE_DEBUG>
        $<$<CONFIG:Release>:RELIABLE_RELEASE>
    )

    foreach(program example soak stats fuzz)
        add_executable(${program} ${program}.c)
        target_link_libraries(${program} PRIVATE reliable)
    endforeach()

    # the libFuzzer harness, built with a standalone driver so it stays healthy in CI.
    # OSS-Fuzz builds the real libFuzzer binary from fuzz_target.c via oss-fuzz/build.sh

    add_executable(fuzz_target_standalone fuzz_target.c)
    target_compile_definitions(fuzz_target_standalone PRIVATE RELIABLE_FUZZ_STANDALONE)
    target_link_libraries(fuzz_target_standalone PRIVATE reliable)

    enable_testing()

    add_test(NAME test COMMAND reliable_test)
    add_test(NAME fuzz COMMAND fuzz 20000 12345)
    add_test(NAME soak COMMAND soak 8192 --quiet)
    add_test(NAME fuzz_target COMMAND fuzz_target_standalone 500 12345)

    # the library must export exactly the RELIABLE_EXPORT surface of reliable.h and nothing
    # else. needs nm to read the symbol table, so it runs where nm exists
    find_program(RELIABLE_NM nm)
    if(RELIABLE_NM)
        add_test(NAME symbols COMMAND ${CMAKE_COMMAND}
            -DRELIABLE_LIBRARY=$<TARGET_FILE:reliable>
            -DRELIABLE_HEADER=${CMAKE_CURRENT_SOURCE_DIR}/reliable.h
            -DRELIABLE_NM=${RELIABLE_NM}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tools/symbols/check_symbols.cmake)
    endif()

    # the header's own promises: const views, and a documented lifetime for every pointer a
    # callback receives
    add_test(NAME header_contract COMMAND ${CMAKE_COMMAND}
        -DRELIABLE_HEADER=${CMAKE_CURRENT_SOURCE_DIR}/reliable.h
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tools/symbols/check_header.cmake)

    # configuring with tests off must not need a C++ compiler
    add_test(NAME configure_without_cxx COMMAND ${CMAKE_COMMAND}
        -S ${CMAKE_CURRENT_SOURCE_DIR}
        -B ${CMAKE_CURRENT_BINARY_DIR}/configure_without_cxx
        -DRELIABLE_BUILD_TESTS=OFF
        -DCMAKE_CXX_COMPILER=reliable-has-no-cxx-compiler)

endif()

# install rules (used by package managers, e.g. the homebrew formula)

include(GNUInstallDirs)

install(TARGETS reliable
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES reliable.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

configure_file(reliable.pc.in reliable.pc @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/reliable.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
