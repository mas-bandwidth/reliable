cmake_minimum_required(VERSION 3.15)

project(reliable VERSION 1.3.5 LANGUAGES C CXX)

# is this the top level project, or pulled in via add_subdirectory / FetchContent?
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(RELIABLE_TOP_LEVEL TRUE)
else()
    set(RELIABLE_TOP_LEVEL FALSE)
endif()

option(RELIABLE_SANITIZE "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(RELIABLE_BUILD_TESTS "Build reliable tests, examples and harnesses" ${RELIABLE_TOP_LEVEL})

# default to debug, same as the old premake makefiles
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
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

if(MSVC)
    add_compile_options(/W4 /fp:fast)
else()
    add_compile_options(-Wall -Wextra -ffast-math)
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

target_include_directories(reliable PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>)

target_compile_definitions(reliable
    PUBLIC
        $<$<CONFIG:Debug>:RELIABLE_DEBUG>
        $<$<CONFIG:Release>:RELIABLE_RELEASE>
)

set_target_properties(reliable PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    WINDOWS_EXPORT_ALL_SYMBOLS ON)

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
