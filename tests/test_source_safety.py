from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
main = (ROOT / "src/main.c").read_text(encoding="utf-8")
ui = (ROOT / "src/apps/apps_ui.c").read_text(encoding="utf-8")
installer = (ROOT / "src/apps/app_installer.c").read_text(encoding="utf-8")
storage = (ROOT / "src/apps/storage.c").read_text(encoding="utf-8")

assert "SifIopReset" not in main + ui + installer + storage
assert "xfrom" not in main.lower() + ui.lower() + installer.lower()
assert "formatHdd" not in main + ui + installer + storage
assert "storage_initialize_existing_system" in storage
assert "fileXioMount" in (
    ROOT / "src/apps/system_version.c"
).read_text(encoding="utf-8")
assert "Hold L1 + R1 and press X to install." in ui
assert "hidden_uninstall_chord" in ui
assert "wait_for_uninstall_confirmation" in ui
assert "app_uninstall_managed_partitions" in installer
assert "APP_MAX_COUNT 16" in (
    ROOT / "include/apps/app_installer.h"
).read_text(encoding="utf-8")
print("standalone application safety: PASS")
