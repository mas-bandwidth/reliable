# OSS-Fuzz integration

This directory contains everything needed to run reliable on
[OSS-Fuzz](https://github.com/google/oss-fuzz), Google's continuous fuzzing
service for open source software. The fuzz entry point itself is
[fuzz_target.c](../fuzz_target.c) in the repository root.

## Submitting to OSS-Fuzz

1. Fork https://github.com/google/oss-fuzz
2. Copy this directory to `projects/reliable/` in the fork
3. Open a pull request. The `primary_contact` in `project.yaml` must be the
   project maintainer's email (it receives bug reports and must be associated
   with a Google account to access the OSS-Fuzz dashboard).

## Testing the integration locally

From an oss-fuzz checkout (requires Docker):

    python infra/helper.py build_image reliable
    python infra/helper.py build_fuzzers --sanitizer address reliable
    python infra/helper.py run_fuzzer reliable reliable_fuzz_target

## Testing the harness without OSS-Fuzz

With any clang that ships libFuzzer:

    clang -g -O1 -fsanitize=fuzzer,address,undefined -DRELIABLE_DEBUG -I. reliable.c fuzz_target.c -o fuzz_target
    ./fuzz_target

Without libFuzzer, the standalone driver runs the harness on random inputs
(this is also built by CMake and runs in CI as the `fuzz_target` ctest):

    cc -g -fsanitize=address,undefined -DRELIABLE_DEBUG -DRELIABLE_FUZZ_STANDALONE -I. reliable.c fuzz_target.c -o fuzz_target_standalone
    ./fuzz_target_standalone 1000 12345
