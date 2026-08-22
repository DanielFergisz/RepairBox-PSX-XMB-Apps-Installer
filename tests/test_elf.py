#!/usr/bin/env python3
import pathlib
import shutil
import subprocess
import sys

if len(sys.argv) != 2:
    raise SystemExit("usage: test_elf.py FILE.elf")
elf = pathlib.Path(sys.argv[1])
nm = shutil.which("mips64r5900el-ps2-elf-nm")
if nm is None:
    raise SystemExit("mips64r5900el-ps2-elf-nm not found")
symbols = subprocess.check_output(
    [nm, "-a", str(elf)], text=True, errors="replace")
raw = elf.read_bytes()

for forbidden in ("sceAtaExecCmd", "SifIopReset", "xfromWrite"):
    assert forbidden not in symbols, forbidden
for required in (
    b"RepairBox.pl PSX XMB Apps Installer v1.0",
    b"PP.APPS-00002..WLE",
    b"PP.APPS-00003..OPL",
    b"PP.APPS-00004..PADTEST",
    b"PP.APPS-00005..POPS",
    b"mass:/PSX_XMB_Apps/WLE-R3Z-DS34",
    b"mass:/PSX_XMB_Apps/OPL",
    b"mass:/PSX_XMB_Apps/PADTEST",
    b"mass:/PSX_XMB_Apps/POPSLOADER",
    b"PS2ICON3D",
    b"pfs0:/version.txt",
):
    assert required in raw, required
assert b"_Report.txt" not in raw
addresses = {}
for line in symbols.splitlines():
    fields = line.split()
    if len(fields) >= 3:
        try:
            addresses[fields[-1]] = int(fields[0], 16)
        except ValueError:
            pass
assert addresses["dma_buffers"] % 256 == 0
assert addresses["copy_buffer"] % 64 == 0
print("Production ELF contract and symbol audit: PASS")
