from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
main = (ROOT / "src/main.c").read_text(encoding="utf-8")
ui = (ROOT / "src/apps/apps_ui.c").read_text(encoding="utf-8")
installer = (ROOT / "src/apps/app_installer.c").read_text(encoding="utf-8")
media = (ROOT / "src/source_media.c").read_text(encoding="utf-8")
makefile = (ROOT / "Makefile").read_text(encoding="utf-8")

assert "source_media_detect_preferred(argc, argv)" in main
assert "source_media_prepare_unrecognized()" in main
assert "apps_ui_run()" in main
assert "storage_initialize_existing_system" in ui
assert "system_version_detect" in ui
assert "app_scan_catalog" in ui
assert "app_install(" in ui
assert "source_media_select(SOURCE_MEDIA_APPS)" in ui
assert "source_media_apps_root()" in (
    ROOT / "include/apps/app_installer.h"
).read_text(encoding="utf-8")
assert "source_media_read_size" in installer
assert "source_media_dread_is_eof" in installer
assert "SEARCH_MAX_DEPTH 2u" in media
assert "USB-MX4SIO" in makefile and "MMCE" in makefile
assert "src/installer" not in makefile
assert "src/psx1_pipeline" not in makefile
assert "src/direct_ready40" not in makefile
assert "src/xfrom_repair" not in makefile

for token in (
    "write_direct_kelf_verified",
    "register_xmb_entry",
    "app_scan_managed_partitions",
    "app_uninstall_managed_partitions",
    'fileXioRename("pfs0:/EXECUTE.NEW", "pfs0:/EXECUTE.KELF")',
    'memcmp(readback_sector, "PS2ICON3D", 9)',
    'memcmp(readback_sector, "PS2X", 4)',
):
    assert token in installer, token
print("standalone application integration: PASS")
