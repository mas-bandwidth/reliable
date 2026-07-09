How to build reliable
=====================

reliable builds with [CMake](https://cmake.org) (3.15 or later) on Windows, MacOS and Linux.

## Building on MacOS and Linux

Go to the command line under the reliable directory and enter:

    cmake -B build -DCMAKE_BUILD_TYPE=Debug
    cmake --build build

Use `-DCMAKE_BUILD_TYPE=Release` for an optimized build.

Run the tests:

    ctest --test-dir build --output-on-failure

Or run binaries directly:

    ./build/bin/test
    ./build/bin/example
    ./build/bin/stats
    ./build/bin/soak
    ./build/bin/fuzz

To build with AddressSanitizer and UndefinedBehaviorSanitizer (recommended when fuzzing):

    cmake -B build -DCMAKE_BUILD_TYPE=Debug -DRELIABLE_SANITIZE=ON

## Building on Windows

You need Visual Studio with the C++ workload installed (CMake is included, or install it separately).

Go to the command line under the reliable directory and enter:

    cmake -B build -A x64
    cmake --build build --config Debug

Use `--config Release` for an optimized build.

Run the tests:

    ctest --test-dir build -C Debug --output-on-failure

Binaries are under `build\bin\Debug` and `build\bin\Release`. You can also open the
generated `build\reliable.sln` in Visual Studio and build/debug from there.

## Continuous integration

Every push and pull request builds debug and release and runs the test suite plus a
bounded fuzz run on Windows (x64), MacOS (Apple Silicon) and Linux (Ubuntu LTS), plus
an ASan/UBSan pass on Linux. See [.github/workflows/ci.yml](.github/workflows/ci.yml).
