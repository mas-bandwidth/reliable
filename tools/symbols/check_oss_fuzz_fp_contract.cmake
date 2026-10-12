cmake_minimum_required(VERSION 3.15)

# Checks the one build path that does not go through CMakeLists.txt.
#
# OSS-Fuzz builds reliable.c and fuzz_target.c through oss-fuzz/build.sh with its own
# $CFLAGS/$CXXFLAGS. Those flags never carry -ffp-contract=off, and GCC's default is
# -ffp-contract=fast (clang's default =on still fuses within an expression), so without
# an explicit flag the fuzzed build contracts float arithmetic and diverges bit-wise from
# every CMake build. This pins -ffp-contract=off onto every compile line in the script.
#
# cmake -DRELIABLE_OSS_FUZZ_BUILD_SCRIPT=<path> -P check_oss_fuzz_fp_contract.cmake

if(NOT RELIABLE_OSS_FUZZ_BUILD_SCRIPT)
    message(FATAL_ERROR "RELIABLE_OSS_FUZZ_BUILD_SCRIPT is required")
endif()

file(READ "${RELIABLE_OSS_FUZZ_BUILD_SCRIPT}" build_script)

string(REPLACE "\n" ";" build_lines "${build_script}")

set(compile_lines 0)
set(failures "")

foreach(line IN LISTS build_lines)
    # a $CC or $CXX compilation is one that passes -c; the $CXX final link line does not
    # compile and needs no codegen flag.
    if(line MATCHES "\\$(CC|CXX) " AND line MATCHES "[-]c ")
        math(EXPR compile_lines "${compile_lines} + 1")
        if(NOT line MATCHES "[-]ffp-contract=off")
            list(APPEND failures "${line}")
        endif()
    endif()
endforeach()

if(compile_lines EQUAL 0)
    list(APPEND failures "no $CC or $CXX compile line found in oss-fuzz/build.sh")
endif()

if(failures)
    foreach(failure IN LISTS failures)
        message(SEND_ERROR "oss-fuzz/build.sh compile line lacks -ffp-contract=off: ${failure}")
    endforeach()
    message(FATAL_ERROR "oss-fuzz/build.sh does not build with -ffp-contract=off")
endif()

message(STATUS "oss-fuzz/build.sh builds every compile line with -ffp-contract=off")
