# reliable 1.0

This document specifies the **wire format** produced and consumed by the
reliable library, precisely enough to write an independent implementation that
interoperates byte-for-byte.

reliable adds packet acknowledgement and fragmentation on top of an unreliable
datagram transport. It does **not** retransmit. It tells you which packets
arrived; what you do about the ones that did not is your business.

## Architecture

Every packet carries a **header** describing which packets the sender has
recently received. Packets larger than a configured threshold are split into
**fragments**, each with its own small header, and reassembled by the receiver.

There are exactly two packet shapes on the wire, distinguished by bit 0 of the
first byte:

| bit 0 of byte 0 | shape |
|---|---|
| `0` | a regular packet: packet header, then payload |
| `1` | a fragment: fragment header, then fragment data |

## General Conventions

All multi-byte integers are **little-endian**: `uint16` is written low byte
first, `uint32` low byte first.

Sequence numbers are 16-bit and **wrap**. All comparisons are modulo 65536; a
difference is computed as `sequence - ack`, and if negative, `65536` is added.

## Packet Header

The header is **variable length, 4 to 9 bytes** — never fewer than 4, because
the prefix byte, the 16-bit sequence and at least a one-byte ack are always
present. The upper bound of 9 is the library's
`RELIABLE_MAX_PACKET_HEADER_BYTES`. Its size within that range depends on how
much of the acknowledgement state can be elided. It carries three values:

* `sequence` — the sequence number of this packet
* `ack` — the most recent sequence number received from the far end
* `ack_bits` — a 32-bit mask of the 32 packets before `ack`, where bit `n` set
  means `ack - n - 1` was received

### The prefix byte

    bit 0       always 0 for a regular packet (1 means fragment)
    bit 1       set if byte 0 of ack_bits is NOT 0xFF
    bit 2       set if byte 1 of ack_bits is NOT 0xFF
    bit 3       set if byte 2 of ack_bits is NOT 0xFF
    bit 4       set if byte 3 of ack_bits is NOT 0xFF
    bit 5       set if (sequence - ack) mod 65536 <= 255
    bits 6-7    unused, zero

The elision is the whole point. In steady state with no loss, every bit of
`ack_bits` is set, so all four bytes are `0xFF`, all four flags are clear, and
none of them are transmitted. A healthy connection carries a 4-byte header; a
lossy one grows toward 9.

Note the flag polarity: **a set flag means the byte is present**. A clear flag
means the byte is absent and the decoder must substitute `0xFF`.

### Layout

    [prefix byte]                       (uint8)
    [sequence]                          (uint16)
    [ack]                               (uint8 if prefix bit 5, else uint16)
    [ack_bits byte 0]                   (uint8, only if prefix bit 1)
    [ack_bits byte 1]                   (uint8, only if prefix bit 2)
    [ack_bits byte 2]                   (uint8, only if prefix bit 3)
    [ack_bits byte 3]                   (uint8, only if prefix bit 4)

When prefix bit 5 is set, the `ack` field is not the sequence number itself but
the **difference** `sequence - ack`, stored in one byte. The decoder recovers
`ack = sequence - difference`. This saves a byte whenever the far end is
current, which is the common case.

`ack_bits` bytes are written **low byte first**, and only those whose flag is
set.

### Canonical encoding is mandatory

A given `(sequence, ack, ack_bits)` triple has exactly **one** legal encoding.
A decoder that accepts a header must be able to re-encode it and get the
identical bytes.

The library enforces this for headers embedded in fragments: it re-encodes what
it read and rejects the packet on any mismatch. An implementation that emits a
non-minimal header — transmitting an `ack_bits` byte that happens to be `0xFF`,
or a 16-bit `ack` when the difference fits in 8 bits — will produce packets
that this library rejects.

## Fragments

A packet larger than the configured `fragment_above` threshold is split into
fragments of `fragment_size` bytes each, up to `max_fragments` (256 maximum).

### Fragment header

Always exactly 5 bytes (`RELIABLE_FRAGMENT_HEADER_BYTES`), followed on
fragment 0 by the embedded packet header:

    [prefix byte]       (uint8)     always exactly 1
    [sequence]          (uint16)    the sequence of the whole packet
    [fragment id]       (uint8)     0-based index of this fragment
    [num fragments - 1] (uint8)     total count, stored minus one

`num_fragments` is stored **minus one**, so a value of `0` means one fragment
and `255` means 256. This is what allows 256 fragments in a single byte.

Every fragment of a packet carries the same `sequence`.

### The embedded packet header

**Fragment 0 only** carries the packet header of the whole packet, immediately
after the fragment header. Later fragments do not. So:

    fragment 0:  [fragment header][packet header][fragment data]
    fragment n:  [fragment header][fragment data]

A receiver therefore learns the acknowledgement state of a fragmented packet
only from fragment 0.

The embedded header is subject to two additional checks: its `sequence` must
equal the fragment header's `sequence`, and it must be canonically encoded, as
above. Both failures reject the packet.

All fragments except the last carry exactly `fragment_size` bytes of data. The
last carries the remainder.

## Receiver Obligations

* Reject a packet too short to contain its header.
* Reject a fragment whose `fragment_id >= num_fragments`, or whose
  `num_fragments` exceeds the configured `max_fragments`.
* Reject a non-canonical embedded header (see above).
* Treat sequence numbers as wrapping; never compare them as plain integers.

None of these are optional. Each is a place where a malformed or hostile packet
would otherwise be accepted.

## What This Format Does Not Do

* **No retransmission.** reliable reports which packets arrived. Resending is
  the caller's decision.
* **No ordering.** Packets are delivered in arrival order.
* **No encryption or authentication.** The header is plaintext and unprotected.
  Anything that needs confidentiality or integrity must layer above or below.
* **No connection state on the wire.** There is no handshake, no connection id,
  and no session. Those belong to a higher layer — netcode, for instance.

## Provenance

Written 2026-07-21 by Rowan, by reading the reference implementation and
verifying every claim differentially against it: headers generated by the
library across a wide range of sequence, ack and ack_bits values were decoded
by an independent implementation written only from this document, and required
to agree on every field and on the exact encoded length.

It documents the format as it stands; where this document and the
implementation disagree, the implementation is authoritative and this document
is a bug.

The reference implementation is `reliable.c`.
