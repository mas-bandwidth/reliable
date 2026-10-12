#!/bin/bash -eu
# build script for OSS-Fuzz (https://github.com/google/oss-fuzz)

$CC $CFLAGS -ffp-contract=off -DRELIABLE_DEBUG -I. -c reliable.c -o reliable.o
$CC $CFLAGS -ffp-contract=off -DRELIABLE_DEBUG -I. -c fuzz_target.c -o fuzz_target.o
$CXX $CXXFLAGS reliable.o fuzz_target.o $LIB_FUZZING_ENGINE -o $OUT/reliable_fuzz_target
