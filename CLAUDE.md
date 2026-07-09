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
ctest --test-dir build --output-on-failure     # runs the test suite + bounded fuzz and soak runs
```

Binaries land in `build/bin` (`build/bin/<Config>` on Windows). Add
`-DRELIABLE_SANITIZE=ON` for ASan+UBSan. CI (`.github/workflows/ci.yml`) runs
Debug+Release on Windows x64, macOS arm64, and Ubuntu LTS, plus a sanitizer job, plus
a weekly 2M-iteration fresh-seed fuzz job under ASan/UBSan (manually triggerable via
workflow_dispatch).

Tests live at the bottom of `reliable.c` behind `RELIABLE_ENABLE_TESTS`, driven by
`test.cpp`. Debug/release is selected by `RELIABLE_DEBUG` / `RELIABLE_RELEASE`; asserts
compile out entirely in release.

**Status (2026-07-09):** premake was replaced by CMake and CI was added in July 2026
(commit `a579740` onward). CI is green across the full matrix, there are no open
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
- **Dated endianness detection** — unlisted architectures (e.g. RISC-V) fell through to
  `RELIABLE_BIG_ENDIAN`. Detection now prefers the compiler's `__BYTE_ORDER__` macro
  (GCC/Clang), keeping the old architecture list as the MSVC fallback. Runtime-verified
  by `test_endian`.
- **Fragment 0 payload could be shifted by a non-canonical packet header** — reassembly
  re-encodes the header canonically, so a non-canonically encoded header (e.g. an 0xFF
  ack_bits block included explicitly) made the re-encoded size differ from the received
  size, shifting where fragment 0's payload landed. No memory-safety impact (bounds
  check from `f0e3be1`), but such packets reassembled corrupted.
  `reliable_read_fragment_header` now rejects fragments whose packet header is not
  byte-identical to the canonical encoding — the library's own sender is always
  canonical, so only forged/corrupt packets are affected.
- **Duplicate packets within the receive window were delivered twice** — per the
  maintainer (2026-07-09) this was an oversight, not design intent.
  `reliable_endpoint_receive_packet` now drops sequences already present in the
  received buffer and counts them in `RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_DUPLICATE`.
  Fragments for already-received sequences are dropped on arrival too, so a replayed
  fragment set costs nothing and a duplicated final fragment cannot spawn a zombie
  reassembly entry. Retransmits of packets the caller rejected (process function
  returned 0) are still processed, because only accepted packets enter the received
  buffer. Covered by `test_duplicate_packets`.

### Scope and operating assumptions (per the maintainer, 2026-07-09 — not issues)

- **Authentication and packet trust are out of scope.** reliable assumes an
  authenticated, encrypted transport beneath it — that is netcode's job (its sister
  library). Attacks that require forging packets (warping the receive window via a fake
  far-future sequence, spoofing acks, injecting payloads) are netcode's problem to
  prevent, not reliable's. Do not add authentication or spoofing defenses here.
- **Fragmentation trades loss amplification for latency, by design.** If any fragment
  is lost the whole packet is lost; in-progress reassemblies evicted by newer traffic
  are the same trade. Time-sensitive delivery comes first. Callers who need large
  blocks delivered reliably should implement block transfer at a higher level (yojimbo
  does this on top of reliable) rather than routinely sending very large packets.
- **Continuous bidirectional packet exchange is assumed.** reliable is designed for
  protocols like action games that send packets both ways at ~60Hz, continually. Acks
  piggyback on outgoing packets, so one-directional or bursty request/response traffic
  is out of scope — no keepalives exist because there are never lulls. Stats
  (rtt/jitter/loss over the recent history window) are fresh under the same assumption.
- **16-bit sequence numbers are sized for this send rate.** At game-style packet rates
  the wrap interval is ample; bulk-transfer rates are out of scope.

### Design notes (intentional; documented in the README "Caveats" section since 2026-07)

- **Acks are dropped once the ack buffer fills** — the caller must call
  `reliable_endpoint_clear_acks` regularly. Since 2026-07 a drop logs at error level
  instead of being silent; the unacked packet can still be reported on a later packet's
  ack bits while it remains within the 32-packet ack window.
- **Not thread-safe.** Endpoints have no locking, and log level / printf / assert
  handlers are process-wide globals. One-endpoint-per-thread or external locking is
  required.

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
