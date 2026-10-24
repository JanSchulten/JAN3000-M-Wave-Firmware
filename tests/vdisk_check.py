#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 M-Vave-OP1 contributors
"""USB DISK's image (tests/vdisk_test.c) read back by a FAT reader of its own, not firmware/src/vdisk.c's logic:

    vdisk_check.py disk.img nor.bin outdir

The MBR's partition, the boot sector (the FAT type from the cluster count, as every reader decides it), both FATs
alike, the root directory, each file followed through its cluster chain into outdir; then every .FM1 (FM1X) checked:
magic, version, size, the entries' CRC, each entry's bytes against its CRC and against nor.bin at its offset; and
README.TXT naming the FM1 files there are. Exit 0 when all of it holds."""
import struct
import sys
import zlib
from pathlib import Path


def fail(msg):
    print(f"vdisk_check: {msg}")
    sys.exit(1)


def main():
    img = Path(sys.argv[1]).read_bytes()
    nor = Path(sys.argv[2]).read_bytes()
    out = Path(sys.argv[3])
    out.mkdir(parents=True, exist_ok=True)
    if img[510:512] != b"\x55\xaa":
        fail("MBR: no signature")
    ptype, start, count = img[446 + 4], *struct.unpack_from("<II", img, 446 + 8)
    if ptype not in (0x04, 0x06, 0x0E) or start == 0 or start + count > len(img) // 512:
        fail(f"MBR: partition type {ptype:#x} at {start} of {count}")
    b = img[start * 512:(start + 1) * 512]
    if b[510:512] != b"\x55\xaa" or b[0] not in (0xEB, 0xE9):
        fail("boot sector: no signature / jump")
    bps, spc, rsvd, nfats, nroot, tot16, media, fatsz, _, _, hidden, tot32 = struct.unpack_from("<HBHBHHBHHHII", b, 11)
    tot = tot16 or tot32
    if bps != 512 or spc == 0 or nfats != 2 or hidden != start or tot != count:
        fail(f"BPB: {bps} {spc} {nfats} hidden {hidden} total {tot}")
    rootsec = (nroot * 32 + bps - 1) // bps
    data0 = rsvd + nfats * fatsz + rootsec
    clusters = (tot - data0) // spc
    if not 4085 <= clusters < 65525:
        fail(f"{clusters} clusters: not a FAT16")
    if b[54:62] != b"FAT16   ":
        fail("BPB: the type string")
    part = img[start * 512:(start + tot) * 512]
    fat = part[rsvd * 512:(rsvd + fatsz) * 512]
    if fat != part[(rsvd + fatsz) * 512:(rsvd + 2 * fatsz) * 512]:
        fail("the two FATs differ")
    ent = lambda c: struct.unpack_from("<H", fat, 2 * c)[0]
    if ent(0) != 0xFF00 | media or ent(1) != 0xFFFF:
        fail("FAT: entries 0, 1")
    root = part[(rsvd + nfats * fatsz) * 512:data0 * 512]
    files, label, used = {}, None, set()
    for i in range(nroot):
        e = root[i * 32:(i + 1) * 32]
        if e[0] == 0:
            break
        if e[0] == 0xE5:
            continue
        attr = e[11]
        name = e[0:8].decode().rstrip() + ("." + e[8:11].decode().rstrip() if e[8:11].strip() else "")
        if attr & 0x08:
            label = e[0:11].decode().rstrip()
            continue
        clus, size = struct.unpack_from("<HI", e, 26)
        data, c, n = bytearray(), clus, 0
        while size and 2 <= c < 0xFFF8:
            if c in used:
                fail(f"{name}: cluster {c} used twice")
            used.add(c)
            s = data0 + (c - 2) * spc
            data += part[s * 512:(s + spc) * 512]
            c = ent(c)
            n += 1
            if n > clusters:
                fail(f"{name}: a loop in its chain")
        if len(data) < size or (size and c < 0xFFF8):
            fail(f"{name}: its chain ends early")
        if not attr & 0x01:
            fail(f"{name}: not read-only")
        files[name] = bytes(data[:size])
        (out / name).write_bytes(files[name])
    if label != "FM-1":
        fail(f"the label {label!r}")
    free = sum(1 for c in range(2, clusters + 2) if ent(c) == 0)
    if free + len(used) != clusters:
        fail("FAT: clusters neither free nor a file's")
    fm1 = sorted(n for n in files if n.endswith(".FM1"))
    readme = files.get("README.TXT", b"").decode("ascii", "replace")
    if not fm1 or any(n not in readme for n in fm1):
        fail(f"README.TXT does not name {fm1}")
    for n in fm1:
        f = files[n]
        magic, ver, cnt, size, slot = struct.unpack_from("<4sHHIB", f, 0)
        tcrc = struct.unpack_from("<I", f, 48)[0]
        if magic != b"FM1X" or ver != 1 or size != len(f) or not 1 <= cnt <= 6 or slot > 4:
            fail(f"{n}: header {magic} {ver} {cnt} {size}/{len(f)} {slot}")
        table = f[64:64 + 16 * cnt]
        if zlib.crc32(table) != tcrc:
            fail(f"{n}: the entries' CRC")
        at = 64 + 16 * cnt
        tags = []
        for i in range(cnt):
            tag, off, ln, crc = struct.unpack_from("<4sIII", table, 16 * i)
            body = f[at:at + ln]
            if len(body) != ln or zlib.crc32(body) != crc:
                fail(f"{n}: {tag} its CRC")
            if body != nor[off:off + ln]:
                fail(f"{n}: {tag} not the flash's bytes at {off:#x}")
            tags.append(tag.decode())
            at += (ln + 3) & ~3
        if at != len(f) or tags[0] != "PROJ":
            fail(f"{n}: {tags}, {at} of {len(f)} bytes")
        print(f"vdisk_check: {n:12} {len(f):7} bytes  slot {slot}  {' '.join(tags)}")
    print(f"vdisk_check: {len(files)} files, {len(used)} of {clusters} clusters used, FAT16, label {label}")


if __name__ == "__main__":
    main()
