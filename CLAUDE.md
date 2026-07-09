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

**Status (2026-07-09):** premake was replaced by CMake and CI was added in July 2026
(commits `a579740`..`9152439`). CI is green across the full matrix, there are no open
issues, and the working tree matches main.

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

The findings from that 2026-07 review are recorded below; everything actionable was
fixed at the time (see "Found and fixed"). What remains is the design contract and the
gotchas — read those before changing anything.

### Design contract: release builds trust the caller (do not "fix" this)

Per the maintainer (2026-07): **correct configuration is the programmer's responsibility
in release.** Debug asserts exist to help the programmer catch bad configuration during
development; release builds deliberately carry no validation overhead. Do not add
release-mode config checks or defensive branches — that would be fighting the library's
design. Concretely, this covers things like `max_fragments > 256` (would overflow
`fragment_received[256]` on the receive path) and inconsistent
`max_packet_size > max_fragments * fragment_size`: real caller errors, caught by asserts
in debug, the caller's problem in release. The same trust model applies to allocators —
the caller supplies them and is responsible for them not failing.

### Open issues

None currently.

### Found and fixed (2026-07, alongside the CMake migration)

- **No CI** — the workflow had been deleted; `.github/workflows/ci.yml` now runs
  Debug+Release build + test suite + bounded fuzz on Windows x64 / macOS arm64 /
  Ubuntu, plus an ASan+UBSan job on Ubuntu.
- **Copy-paste bug in a test** — `test_acks_packet_loss` fetched the *sender's* acks
  twice and never checked the receiver's; reliable.c:2155 now checks `context.receiver`.
- **README sample code bugs** — the ack loop indexed `acks[j]` with loop variable `i`,
  and the stats `printf` was missing its opening quote.
- **Six allocation results lacked the debug assert** that the rest of the code applies
  (sequence buffer struct, endpoint acks + rtt history buffers, both send-path scratch
  buffers, and the fragment reassembly buffer). All now have `reliable_assert`, keeping
  release behavior untouched per the design contract.

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
- Error handling philosophy is assert-in-debug, trust-the-caller-in-release. This is
  the maintainer's explicit design contract (see above), not an accident.
- Test coverage of the happy paths is good; adversarial coverage lives in `fuzz.c`,
  which is genuinely well constructed (loss, reorder, duplication, bit corruption,
  random injection over a simulated link).

### Bottom line

A tight, battle-tested library that does one thing well, with real fuzzing and a recent
security pass behind it. The code earns its "production ready" claim. Release builds
trusting the caller on configuration and allocation is the maintainer's explicit design
contract — respect it. No open issues as of 2026-07.
