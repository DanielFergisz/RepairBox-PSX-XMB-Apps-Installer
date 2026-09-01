# RepairBox.pl PSX XMB App Installer v1.0

This installer adds PS2 ELF applications to the Games section of the XMB on a
Sony PSX DESR. It works with PSX1 and PSX2 systems and detects the installed
revision from `__system/version.txt`.

It is made for a PSX with a working system that has already completed its first
XMB boot. It does not format or repartition the HDD.

## USB layout

Put the installer ELF next to a folder named `PSX_XMB_Apps`. Each application
gets its own folder:

```text
mass:/
  RepairBox.pl-PSX-XMB-Dynamic-App-Installer-v1.0.elf
  PSX_XMB_Apps/
    My Application/
      MyApp.ELF
      cover.png       optional
      app.ini         optional
```

For a simple package, one folder with one ELF is enough. The folder name is
used as the XMB title and stable application ID.

Use `app.ini` when you want a different title, a fixed ID, or need to choose
one ELF from a folder:

```ini
id=my-application
title=My Application
subtitle=Homebrew Utility
elf=MyApp.ELF
```

Keep `id` unchanged when publishing an update. Running the installer again
then updates the same XMB entry instead of creating another copy.

## Requirements and limits

- Start the installer with **wLaunchELF v4.70_R3Z**.
- Up to 16 application folders are scanned in one run.
- Each application uses a dedicated 128 MiB PFS partition.
- ELF files must be 32-bit little-endian MIPS executables.
- The maximum ELF size is 2,025,312 bytes.
- `cover.png` may be 16x16 through 256x256 and up to 1 MiB; 72x112 works well.
- Invalid packages are marked as `ERROR` and skipped.

## Installing and updating

1. Copy the installer ELF and the complete `PSX_XMB_Apps` folder to USB.
2. Run the installer from wLaunchELF v4.76_R3Z.
3. Check the detected revision and package list.
4. Hold `L1 + R1` and press `X`.
5. Wait for installation and verification to finish.
6. Fully power off the PSX, disconnect AC power, reconnect it and boot XMB.

Removing an application folder from USB does not uninstall its XMB entry.

## Maintenance uninstall

The installer can remove manually installed application partitions that pass
its structural safety checks. From the package list, hold
`L1 + R1 + L2 + R2` and press `TRIANGLE`, then follow the confirmation shown on
screen. This removes every verified manual application found by the scan, so
use it carefully.

## Checking packages on a PC

Python 3 can validate the folder layout before copying it to USB:

```sh
python tools/validate_usb_packages.py PSX_XMB_Apps
```

## Building

A working PS2DEV/PS2SDK environment is required:

```sh
export PS2DEV=/path/to/ps2dev
export PS2SDK="$PS2DEV/ps2sdk"
export PATH="$PS2DEV/bin:$PS2DEV/ee/bin:$PS2DEV/iop/bin:$PS2SDK/bin:$PATH"
make clean all
```

You can also run `sh tools/build_local.sh` after setting `PS2DEV` and `PS2SDK`.
The output is `RepairBox.pl-PSX-XMB-Dynamic-App-Installer-v1.0.elf`. A verified
prebuilt ELF and its SHA-256 manifest are included in `release/`.

Version 1.0 does not create diagnostic reports.
