# Security Policy

reliable implements packet fragmentation, reassembly and acknowledgement over an
unreliable transport. It reassembles untrusted fragments arriving off the wire into
larger buffers, which makes memory safety the primary concern here.

## Reporting a vulnerability

**Please do not report security issues in public GitHub issues or pull requests.**

Report privately through either channel:

- **GitHub private vulnerability reporting** (preferred): on this repository, go to the
  **Security** tab → **Report a vulnerability**. This opens a private advisory visible only
  to the maintainers.
- **Email**: glenn@mas-bandwidth.com.

Please include enough detail to reproduce: the affected component and version/commit, a
description of the flaw, and — where possible — a proof-of-concept input or a small patch.
Fuzzing crash artifacts (a crashing input file plus the target name) are ideal.

We will acknowledge your report, keep you updated on our assessment, and coordinate
disclosure timing with you. We prefer coordinated disclosure and will credit reporters who
wish to be named.

## Scope

In scope — bugs in the reliable library itself (`reliable.c`, `reliable.h`).

Especially of interest: memory-safety issues reachable from a received packet — the
fragment reassembly path above all, where a malicious peer controls fragment indices,
counts and sizes. Out-of-bounds read/write, integer overflow in size arithmetic, and
resource exhaustion through partially-completed reassembly buffers are the shapes we
most want to hear about.

reliable performs **no encryption and no authentication**. It is designed to sit under a
layer that does (netcode, in yojimbo's case). A deployment that exposes reliable directly
to untrusted peers without that layer is outside its threat model — but memory-safety
bugs are still bugs, and we want them.

## Supported versions

Security fixes land on the latest release. We do not backport to older release lines.
