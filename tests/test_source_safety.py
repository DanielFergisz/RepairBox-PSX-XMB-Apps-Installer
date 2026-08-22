#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sources = "\n".join(
    path.read_text(encoding="utf-8", errors="replace")
    for path in sorted((ROOT / "src").glob("*.c"))
)
installer = (ROOT / "src" / "app_installer.c").read_text(encoding="utf-8")
main = (ROOT / "src" / "main.c").read_text(encoding="utf-8")
version = (ROOT / "src" / "system_version.c").read_text(encoding="utf-8")

for forbidden in (
    'fileXioFormat("hdd0:"',
    "HDIOC_SETMAXLBA28",
    "HDIOC_SETMAXLBA48",
    "dvr_hdd0:",
    "XFROM",
    "xfrom",
    "SifIopReset",
):
    assert forbidden not in sources, forbidden

assert sources.count("HDIOC_WRITESECTOR") == 1
assert '"hdd0:", HDIOC_WRITESECTOR' in installer
assert '"hdd0:%s,,,128M,PFS"' in installer
assert 'fileXioFormat(\n            "pfs:", blockdev' in installer
assert "partition_start + 8u" in installer
assert "partition_start + 9u" in installer
assert "partition_start + 10u" in installer
assert 'memcmp(icon_sys, "PS2X", 4)' in installer
assert 'header->type != 0x0100u' in installer
assert "APP_PARTITION_SECTORS 0x00040000u" in installer
assert "APP_PFS_ZONE_SIZE 8192" in installer
assert "hash_text_equal(diagnostic->source_sha256" in installer
assert "hash_text_equal(diagnostic->target_sha256" in installer
assert "APP_COUNT 4" in (ROOT / "include" / "app_installer.h").read_text(encoding="utf-8")
for partition in (
    "PP.APPS-00002..WLE",
    "PP.APPS-00003..OPL",
    "PP.APPS-00004..PADTEST",
    "PP.APPS-00005..POPS",
):
    assert partition in installer

assert "select_revision" not in main
assert "system_version_detect(&version)" in main
assert "storage_initialize_existing_system(&storage)" in main
assert main.index("storage_initialize_existing_system(&storage)") < main.index("system_version_detect(&version)")
assert "result->filexio_init_result = fileXioInit()" not in version
assert '"hdd0:__system"' in version
assert '"pfs0:/version.txt"' in (ROOT / "include" / "system_version.h").read_text(encoding="utf-8")
assert "installer_report_save" not in sources
assert "PAD_SQUARE" not in main
assert "SQUARE Save report" not in main
assert "APP_COUNT * APP_STEP_COUNT" in main
assert "PROGRESS_REFRESH_MS 500u" in main
assert "if (!state->screen_ready)" in main
assert "elapsed - state->last_draw_ms < PROGRESS_REFRESH_MS" in main
assert "progress_line(88, line)" in main
assert "progress_line(168, line)" in main
print("Production targeted-write and stable-progress safety audit: PASS")
