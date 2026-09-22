# RepairBox.pl PSX XMB App Installer v1.1

Install PS2 ELF applications in the **Games** section of the Sony PSX DESR XMB. The installer supports PSX1 and PSX2 and detects the revision automatically. It needs a working system that has already completed its first XMB boot; it does not format or repair the HDD.

## Download

**[Download the ready-to-use USB package with five apps](https://drive.google.com/file/d/1wCknVuOCgDW_a-4R33TOQH8lCR5DSQmj/view?usp=sharing)** (`RepairBox.pl-PSX_XMB_Apps_v1.1.zip`)

The package includes the USB/MX4SIO installer, a wLaunchELF R3Z ELF, and:

- DirectDisc
- LaunchELF (wLaunchELF R3Z)
- OPL v1.2.1 — adapted for PSX DVR with improved MX4SIO support
- PadTest
- POPSLoader

For your own application collection, use the appropriate installer from [`release/`](release/):

| Launch medium | Installer |
| --- | --- |
| USB or MX4SIO | `RepairBox.pl-PSX-XMB-Dynamic-App-Installer-v1.1-USB-MX4SIO.elf` |
| MMCE (slot 1 or 2) | `RepairBox.pl-PSX-XMB-Dynamic-App-Installer-v1.1-MMCE.elf` |

The ready-to-use ZIP contains the USB/MX4SIO build under a shorter filename. Use the MMCE build when launching from MMCE.

## Installation

1. Extract the ready-to-use package to a USB drive, or put your chosen installer ELF beside a `PSX_XMB_Apps` folder on the same medium.
2. Launch the installer with wLaunchELF v4.78_R3Z or an equivalent PSX-compatible build. Check the detected revision and application list.
3. Hold `L1 + R1` and press `X`. Wait for copying and verification to finish.
4. Fully power off the PSX, disconnect AC power, then reconnect and boot XMB.

Each new app needs 128 MiB of free HDD space; all five prepared apps need at least 640 MiB. 

## Add or update an app

Create one folder per application inside `PSX_XMB_Apps`. A folder with one ELF is enough; `cover.png` and `app.ini` are optional:

```text
PSX_XMB_Apps/
  My Application/
    MyApp.ELF
    cover.png
    app.ini
```

Use `app.ini` if you want a custom title or a stable ID for updates:

```ini
id=my-application
title=My Application
elf=MyApp.ELF
```

Keep the same `id` when replacing an app. Removing its folder from the source medium does not uninstall the XMB entry. The installer scans up to 16 folders per run; each ELF must be a 32-bit little-endian MIPS executable no larger than 2,025,312 bytes. You can check packages on a PC with `python3 tools/validate_usb_packages.py PSX_XMB_Apps`.

The maintenance shortcut `L1 + R1 + L2 + R2 + TRIANGLE` opens a separate confirmation to remove **all** verified manually installed application partitions. It is not a single-app uninstall.

## Build

With PS2DEV/PS2SDK v2.0.0 and Python 3 configured, run `make all` to build both variants and run the host checks. Prebuilt ELFs and their SHA-256 checksums are in [`release/`](release/).
