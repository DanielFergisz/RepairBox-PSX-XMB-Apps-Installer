from pathlib import Path
import sys

if len(sys.argv) != 3:
    raise SystemExit("usage: check_release.py FLAVOR FILE.elf")

flavor, name = sys.argv[1:]
root = Path(__file__).resolve().parents[1]
raw = Path(name).read_bytes()
assert raw[:4] == b"\x7fELF"
assert b"PSX XMB App Installer v1.1" in raw
assert b"PSX HDD + Apps Setup" not in raw
# The shared read-only media resolver still carries system-package names.
# No system installer, formatter or XFROM writer is linked into this ELF.
assert b"SifIopReset" not in raw
assert (root / "irx/psx1/ps2hdd-sparse-skip.irx").read_bytes() in raw

if flavor == "MMCE":
    assert b"MMCE" in raw
    assert (root / "irx/mmceman.irx").read_bytes() in raw
    for forbidden in ("bdm", "bdmfs_fatfs", "mx4sio_bd", "usbd", "usbmass_bd"):
        assert (root / "irx/pinned" / (forbidden + ".irx")).read_bytes() not in raw
elif flavor == "USB-MX4SIO":
    assert b"USB/MX4SIO" in raw
    assert (root / "irx/mmceman.irx").read_bytes() not in raw
    for required in ("bdm", "bdmfs_fatfs", "mx4sio_bd", "usbd", "usbmass_bd"):
        assert (root / "irx/pinned" / (required + ".irx")).read_bytes() in raw
else:
    raise AssertionError(flavor)
print(f"{flavor} release profile: PASS")
