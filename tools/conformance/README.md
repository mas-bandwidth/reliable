# Conformance: STANDARD.md vs the implementation

`STANDARD.md` specifies reliable's wire format. `reliable.c` implements it. If
they drift, independent implementations built from the document are wrong and
nobody finds out until two of them refuse to talk.

This checks them against each other. `gen_vectors.c` links the real library and
emits artifacts; `verify_standard.py` parses them using **only what STANDARD.md
says**, with no reference to `reliable.c`, and asserts every field.

    python3 tools/conformance/verify_standard.py

Exit 0 means the document and the code agree. A failure means one of them is
wrong — decide which, and fix that one.

## What is covered

* **packet headers** — 512 vectors across the corners of the elision rules:
  sequence/ack wrap, difference at exactly 255 and 256, `ack_bits` all-set,
  none-set and mixed. Every field plus the **exact byte length**, because a
  subtly wrong elision rule still decodes correctly while consuming the wrong
  number of bytes and desynchronizing everything after it.
* **fragment headers** — real fragments from a real endpoint: the prefix byte
  of exactly 1, fragment ids, `num_fragments` stored minus one, the shared
  sequence, that **only fragment 0 carries the embedded packet header**, and
  that data sizes are `fragment_size` except for the remainder in the last.

## What is NOT covered

Endpoint behaviour above the wire: ack logic, reassembly state, RTT and
counters. Those are the library's semantics rather than its format, and
`test.cpp` is where they belong.
