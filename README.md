# RepairBox.pl PSX XMB App Installer v1.1

This standalone installer adds PS2 ELF applications to the Games section of
the XMB on Sony PSX DESR systems. It supports PSX1 and PSX2 and detects the
installed revision from `__system/version.txt`.

Use it only on a PSX with a working HDD system that has completed its first
XMB boot. It **does not** install, format, repartition, or repair the system.
Back up important data before modifying HDD application partitions.

## Choose the correct ELF

Two separate builds are included in [`release/`](release/):

| Launch medium | Installer ELF |
| --- | --- |
| USB or MX4SIO | `RepairBox.pl-PSX-XMB-Dynamic-App-Installer-v1.1-USB-MX4SIO.elf` |
| MMCE (slot 1 or 2) | `RepairBox.pl-PSX-XMB-Dynamic-App-Installer-v1.1-MMCE.elf` |

Do not launch the MMCE build from USB, or the USB/MX4SIO build from MMCE.
Launch with **wLaunchELF v4.70_R3Z** or the equivalent environment used for
the RepairBox HDD + Apps v1.3 installer.

## Application package layout

Put the chosen ELF and `PSX_XMB_Apps` together on the same medium. They may
be at its root or together in an ordinary subdirectory. The installer checks
the ELF's location and its ancestors, then performs a bounded search up to
two directory levels when necessary. Avoid multiple `PSX_XMB_Apps` folders on
one medium: an ambiguous match will not be selected.

```text
RepairBox/
  RepairBox.pl-PSX-XMB-Dynamic-App-Installer-v1.1-USB-MX4SIO.elf
  PSX_XMB_Apps/
    My Application/
      MyApp.ELF
      cover.png       optional
      app.ini         optional
```

For a simple package, use one folder with exactly one ELF. The folder name
becomes the default XMB title and stable application ID. Use `app.ini` to set
them explicitly or select an ELF when the folder contains more than one:

```ini
id=my-application
title=My Application
subtitle=Homebrew Utility
elf=MyApp.ELF
```

Keep `id` unchanged when publishing an update. Running the installer again
then updates the same XMB entry. Removing an application folder from the
source medium does **not** uninstall its XMB entry.

## Requirements and limits

- Up to 16 application folders are scanned in one run.
- Each application uses a dedicated 128 MiB PFS partition on the PSX HDD.
- The input must be a 32-bit little-endian MIPS ELF, at most **2,025,312
  bytes**. This is the payload limit imposed by the generated KELF.
- Optional `cover.png` must be 16×16 through 256×256 pixels and no larger
  than 1 MiB. A 72×112 cover works well.
- Invalid packages are marked `ERROR` and skipped.
- This release displays status on the console; it does not create a
  diagnostic report file.

## Install or update

1. Copy the chosen installer ELF and your complete `PSX_XMB_Apps` folder to
   the same USB or MMCE medium.
2. Run the ELF in wLaunchELF and check the detected PSX revision and package
   list.
3. Hold `L1 + R1` and press `X` to install the valid packages. `TRIANGLE`
   rescans the source; `O` exits.
4. Wait for installation and verification to finish.
5. Fully power off the PSX, disconnect AC power, reconnect it, and boot XMB.

The order of entries in XMB is not controlled by the order of source folders.

### Maintenance uninstall

From the package list, hold `L1 + R1 + L2 + R2` and press `TRIANGLE` to enter
the separate maintenance confirmation. This removes **every** manually added
application partition that passes the installer's structural safety checks;
it is not a single-app uninstall. The operation is not automatically
reversible. Do not use it for normal updates.

## Check packages on a PC

Python 3 can validate package layout and the ELF size before copying to the
console:

```sh
python3 tools/validate_usb_packages.py PSX_XMB_Apps
```

The PC check is a preflight aid, not a substitute for the installer's checks
or a successful console test.

## Build from source

The project requires a working PS2DEV/PS2SDK v2.0.0 environment with the
PS2SDK ports headers and Python 3. For example:

```sh
export PS2DEV=/path/to/ps2dev
export PS2SDK="$PS2DEV/ps2sdk"
export PATH="$PS2DEV/bin:$PS2DEV/ee/bin:$PS2DEV/iop/bin:$PS2SDK/bin:$PATH"
make all
```

`make all` runs the host checks, builds both profiles, and checks that their
embedded drivers are separated correctly. The two generated ELFs appear in
the project root. Prebuilt binaries in `release/` are accompanied by
`release/SHA256SUMS.txt`; verify them before distributing or installing.

The application-installation code is based on RepairBox HDD + Apps v1.3. The
standalone entry point and media-specific build profiles are new in v1.1.
See [third-party notices](THIRD_PARTY_NOTICES.md) for bundled components and
their licenses. No Sony system files or application payloads are included.
