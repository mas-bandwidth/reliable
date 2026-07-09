# CLAUDE.md

## What this is

**reliable** is a single-file C library (`reliable.c` / `reliable.h`, ~2,600 lines
including embedded tests) implementing packet acknowledgement, fragmentation/reassembly,
and RTT/jitter/packet-loss/bandwidth estimation over UDP. It is transport-agnostic: the
caller supplies `transmit_packet_function` / `process_packet_function` callbacks. The
other files (`test.cpp`, `example.c`, `soak.c`, `stats.c`, `fuzz.c`) are thin harnesses
around the library.

Build and test (CMake, 3.15+):

```
cmake -B build -DCMAKE_BUILD_TYPE=Debug        # or Release; on Windows: cmake -B build -A x64
cmake --build build                            # on Windows add: --config Debug
ctest --test-dir build --output-on-failure     # runs the test suite + a bounded fuzz run
```

Binaries land in `build/bin` (`build/bin/<Config>` on Windows). Add
`-DRELIABLE_SANITIZE=ON` for ASan+UBSan. CI (`.github/workflows/ci.yml`) runs
Debug+Release on Windows x64, macOS arm64, and Ubuntu LTS, plus a sanitizer job.

Tests live at the bottom of `reliable.c` behind `RELIABLE_ENABLE_TESTS`, driven by
`test.cpp`. Debug/release is selected by `RELIABLE_DEBUG` / `RELIABLE_RELEASE`; asserts
compile out entirely in release.

## Honest assessment

### Overall

This is mature, production-quality code written by someone who knows exactly what they
are doing. The design is small, focused, and allocation-disciplined; the wire format is
compact and well thought out (variable-length ack encoding via prefix byte); the
sequence-buffer data structure is the right tool and is implemented correctly, including
16-bit wraparound. The recent security-audit pass shows: header parsing bounds-checks
every read, fragment reassembly validates sizes before copying
(`reliable_store_fragment_data`, reliable.c:1071-1082), and the fuzzer exercises the
real send→fragment→corrupt→reassemble path rather than just throwing random bytes at
`receive`. I compiled with `-Wall -Wextra` (clean), ran the full test suite (passes),
and ran 20k fuzz iterations under ASan+UBSan (clean).

That said, it is not flawless. Specific findings below, roughly in order of importance.

### Real issues

1. **No CI.** ~~`.github/` contains only `FUNDING.yml`; the workflow was deleted (commit
   `1143edf "fuck github actions"`).~~ **Fixed 2026-07:** the project now builds with
   CMake and `.github/workflows/ci.yml` runs Debug+Release build + test suite + bounded
   fuzz on Windows x64 / macOS arm64 / Ubuntu, plus an ASan+UBSan job on Ubuntu.

2. **Copy-paste bug in a test** — ~~`test_acks_packet_loss` fetches the *sender's* acks
   twice; the receiver's acks are never checked.~~ **Fixed 2026-07** (reliable.c:2155
   now checks `context.receiver`).

3. **Allocation failure is not handled.** Allocator results are either asserted
   (compiled out in release) or not checked at all: `reliable_sequence_buffer_create`
   writes through the struct pointer unconditionally (reliable.c:169-183), and the
   fragment reassembly buffer allocation at reliable.c:1258 is never checked, even in
   debug — a failed `malloc` under memory pressure means a NULL deref on attacker-timed
   input. Given custom allocators are a first-class feature (fixed pools that *can* run
   dry), returning/ignoring on allocation failure would be more defensible than crashing.

4. **Config validation is assert-only.** In release builds nothing stops
   `max_fragments > 256` (overflows `fragment_received[256]` on the receive path since
   `reliable_read_fragment_header` validates against the *configured* max), or an
   inconsistent `max_packet_size > max_fragments * fragment_size` (sender emits more
   fragments than the receiver accepts). These are caller errors, but they fail silently
   or memory-unsafely in release. A few real runtime checks in
   `reliable_endpoint_create` would be cheap.

5. **README sample code has bugs**: ~~the ack loop indexes `acks[j]` with loop variable
   `i`, and the stats `printf` is missing its opening quote.~~ **Fixed 2026-07.**

### Design gotchas (intentional, but undocumented — callers must know)

- **Duplicate packets within the window are delivered twice.**
  `reliable_endpoint_receive_packet` only rejects *stale* sequences
  (`reliable_sequence_buffer_test_insert` checks age, not prior receipt), so a duplicated
  or replayed packet inside the window reaches `process_packet_function` again.
  Deduplication/replay protection is deliberately the caller's job (netcode provides it),
  but neither README nor header says so.
- **Acks are silently dropped once the ack buffer fills** (reliable.c:1170). If the
  caller doesn't call `reliable_endpoint_clear_acks` regularly, delivered packets are
  never reported acked — which in a message layer means spurious resends, not errors.
- **Not thread-safe.** Endpoints have no locking, and log level / printf / assert
  handlers are process-wide globals. One-endpoint-per-thread or external locking is
  required.
- **The endianness detection list is dated** (reliable.h:43-56): unlisted architectures
  (e.g. RISC-V) fall through to `RELIABLE_BIG_ENDIAN`. Harmless in practice — the
  serialization code is explicit byte-order and only `test_endian` uses the macro — but
  it looks load-bearing when it isn't.
- **Fragment 0's packet header is re-encoded canonically during reassembly**
  (`reliable_store_fragment_data`). An attacker sending a non-canonically encoded header
  can shift where payload bytes land for fragment 0. The bounds check added in `f0e3be1`
  confines this to corrupting the reassembled packet's own contents (which an attacker
  who can forge packets could do anyway) — no memory-safety impact, just worth knowing
  the invariant "re-encoded header size == received header size" only holds for
  canonical senders.

### Style notes

- The code is consistently C89-flavored C99 with the author's idiosyncratic spacing.
  Match it when editing; don't "modernize."
- Error handling philosophy is assert-in-debug, trust-the-caller-in-release. Consistent,
  but see items 3 and 4 above for where it bites.
- `test_acks_packet_loss` aside, test coverage of the happy paths is good; adversarial
  coverage lives in `fuzz.c`, which is genuinely well constructed (loss, reorder,
  duplication, bit corruption, random injection over a simulated link).

### Bottom line

A tight, battle-tested library that does one thing well, with real fuzzing and a recent
security pass behind it. The code earns its "production ready" claim. The one open
decision: whether release builds should validate config and allocation failures instead
of trusting asserts that no longer exist (items 3 and 4 above). CI, the test bug, and
the README samples were fixed 2026-07 alongside the CMake migration.
