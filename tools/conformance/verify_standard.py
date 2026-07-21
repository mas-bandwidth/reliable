#!/usr/bin/env python3
"""Check reliable/STANDARD.md against the implementation.

Parses artifacts produced by the real library using ONLY what STANDARD.md
states. Nothing here consults reliable.c. If the document and the code
disagree, this fails and one of them is wrong.

usage: python3 tools/conformance/verify_standard.py [--cc CC]
exit:  0 = they agree, 1 = they do not, 2 = could not build/run
"""
import argparse, os, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def build_and_run(cc):
    src = os.path.join(ROOT, "tools", "conformance", "gen_vectors.c")
    with tempfile.TemporaryDirectory() as tmp:
        exe = os.path.join(tmp, "gen")
        r = subprocess.run([cc, "-I" + ROOT, "-o", exe, src, os.path.join(ROOT, "reliable.c")],
                           capture_output=True, text=True)
        if r.returncode != 0:
            print("build failed:\n" + r.stderr[:2000], file=sys.stderr); sys.exit(2)
        r = subprocess.run([exe], capture_output=True, text=True)
        if r.returncode != 0:
            print("generator failed:\n" + r.stderr[:2000], file=sys.stderr); sys.exit(2)
        return r.stdout


class Checker:
    def __init__(self): self.n = 0; self.fails = []
    def eq(self, name, got, exp):
        self.n += 1
        if got != exp: self.fails.append(f"{name}: got {got!r}, STANDARD.md says {exp!r}")


def decode_packet_header(b):
    """STANDARD.md, 'Packet Header'. Returns (sequence, ack, ack_bits, bytes_consumed)."""
    i = 0
    prefix = b[i]; i += 1
    if prefix & 1:
        raise ValueError("bit 0 set: this is a fragment, not a regular packet")
    seq = b[i] | (b[i + 1] << 8); i += 2
    if prefix & (1 << 5):                       # ack stored as a one-byte difference
        ack = (seq - b[i]) & 0xFFFF; i += 1
    else:
        ack = b[i] | (b[i + 1] << 8); i += 2
    ack_bits = 0
    for n, flag in enumerate((1, 2, 3, 4)):     # a SET flag means the byte is PRESENT
        if prefix & (1 << flag):
            ack_bits |= b[i] << (8 * n); i += 1
        else:
            ack_bits |= 0xFF << (8 * n)
    return seq, ack, ack_bits, i


def decode_fragment_header(b):
    """STANDARD.md, 'Fragments'. Returns (sequence, fragment_id, num_fragments, consumed)."""
    i = 0
    prefix = b[i]; i += 1
    if prefix != 1:
        raise ValueError(f"fragment prefix byte must be exactly 1, got {prefix}")
    seq = b[i] | (b[i + 1] << 8); i += 2
    frag_id = b[i]; i += 1
    num_frags = b[i] + 1; i += 1               # stored MINUS ONE
    return seq, frag_id, num_frags, i


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--cc", default=os.environ.get("CC", "cc"))
    a = ap.parse_args()
    c = Checker()
    frags = []; fraginfo = None

    for line in build_and_run(a.cc).splitlines():
        f = line.split()
        if f[0] == "HDR":
            seq, ack, bits, raw = int(f[1]), int(f[2]), int(f[3]), bytes.fromhex(f[4])
            ds, da, dab, dn = decode_packet_header(raw)
            c.eq(f"header seq={seq} ack={ack} bits={bits:#010x}: sequence", ds, seq)
            c.eq(f"header seq={seq} ack={ack} bits={bits:#010x}: ack", da, ack)
            c.eq(f"header seq={seq} ack={ack} bits={bits:#010x}: ack_bits", dab, bits)
            c.eq(f"header seq={seq} ack={ack} bits={bits:#010x}: consumed all bytes", dn, len(raw))
            c.eq(f"header seq={seq} ack={ack} bits={bits:#010x}: length within 4..9",
                 4 <= len(raw) <= 9, True)
        elif f[0] == "FRAG":
            frags.append(bytes.fromhex(f[2]))
        elif f[0] == "FRAGINFO":
            fraginfo = (int(f[1]), int(f[2]))

    # ---- fragments
    payload_bytes, fragment_size = fraginfo
    expected_frags = (payload_bytes + fragment_size - 1) // fragment_size
    c.eq("number of fragments emitted", len(frags), expected_frags)

    for idx, raw in enumerate(frags):
        seq, fid, nfrags, consumed = decode_fragment_header(raw)
        c.eq(f"fragment {idx}: fragment header is exactly 5 bytes", consumed, 5)
        c.eq(f"fragment {idx}: prefix byte is 1", raw[0], 1)
        c.eq(f"fragment {idx}: fragment id", fid, idx)
        c.eq(f"fragment {idx}: num_fragments (stored minus one)", nfrags, expected_frags)
        c.eq(f"fragment {idx}: same sequence as fragment 0", seq, decode_fragment_header(frags[0])[0])
        if idx == 0:
            # STANDARD.md: fragment 0 ALONE carries the packet header
            hs, ha, hab, hn = decode_packet_header(raw[consumed:])
            c.eq("fragment 0: embedded header sequence equals fragment sequence", hs, seq)
            data = len(raw) - consumed - hn
            c.eq("fragment 0: data is exactly fragment_size", data, fragment_size)
        else:
            data = len(raw) - consumed
            expect = fragment_size if idx < expected_frags - 1 else payload_bytes % fragment_size
            c.eq(f"fragment {idx}: data size", data, expect)
            # later fragments must NOT carry a packet header
            c.eq(f"fragment {idx}: no embedded header (size accounts for data alone)",
                 len(raw), consumed + expect)

    print(f"{c.n} checks against STANDARD.md, {len(c.fails)} failures")
    for x in c.fails[:15]: print("  FAIL " + x)
    if c.fails:
        print("\nSTANDARD.md and the implementation disagree. One of them is wrong.")
        return 1
    print("\nSTANDARD.md matches the implementation.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
