cmake_minimum_required(VERSION 3.15)

project(reliable C CXX)

option(RELIABLE_SANITIZE "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

# default to debug, same as the old premake makefiles
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release)
endif()

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

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

add_library(reliable STATIC reliable.c reliable.h)

target_include_directories(reliable PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(reliable
    PUBLIC
        $<$<CONFIG:Debug>:RELIABLE_DEBUG>
        $<$<CONFIG:Release>:RELIABLE_RELEASE>
    PRIVATE
        RELIABLE_ENABLE_TESTS=1
)

# "test" is a reserved target name in cmake, so the target is reliable_test
# but the binary is still bin/test

add_executable(reliable_test test.cpp)
set_target_properties(reliable_test PROPERTIES OUTPUT_NAME test)
target_link_libraries(reliable_test PRIVATE reliable)

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
