## RepairBox.pl PSX XMB App Installer v1.1

Standalone installer for adding PS2 ELF applications to the Games section of the XMB on Sony PSX DESR systems. It supports PSX1 and PSX2 and detects the installed revision automatically.

Applications are supplied as folders in `PSX_XMB_Apps`; they are not hard-coded into the installer. Version 1.1 separates the USB/MX4SIO and MMCE drivers into two dedicated builds.

### Included installers

- `RepairBox.pl-PSX-XMB-Dynamic-App-Installer-v1.1-USB-MX4SIO.elf` — launch from USB or MX4SIO.
- `RepairBox.pl-PSX-XMB-Dynamic-App-Installer-v1.1-MMCE.elf` — launch from MMCE in slot 1 or 2.

Use the build that matches the device from which you launch the ELF. The main release archive does **not** include application packages; supply your own or use the optional USB bundle below.

### Ready-to-use USB bundle

The separate [RepairBox.pl-PSX_XMB_Apps_v1.1.zip](https://drive.google.com/file/d/1wCknVuOCgDW_a-4R33TOQH8lCR5DSQmj/view?usp=sharing) contains the USB/MX4SIO installer, a wLaunchELF R3Z ELF, and five prepared application folders:

- DirectDisc
- LaunchELF (wLaunchELF R3Z)
- OPL (Open PS2 Loader)
- PadTest
- POPSLoader

Extract the bundle to a USB drive and launch its installer from a compatible wLaunchELF environment. The included installer is the USB/MX4SIO build; for MMCE, use the dedicated MMCE ELF from the main release instead. You can also add your own compatible applications under `PSX_XMB_Apps`.

### Requirements

- A Sony PSX DESR with a working HDD system that has completed its first XMB boot.
- wLaunchELF v4.70_R3Z or an equivalent environment used with RepairBox HDD + Apps v1.3.
- 128 MiB of unallocated HDD space for each new application.

This installer does not install, format, repartition, or repair the PSX system.

### Application layout

Place the selected installer ELF and `PSX_XMB_Apps` together on the same medium. They may be at its root or together in a subdirectory.

```text
RepairBox/
  RepairBox.pl-PSX-XMB-Dynamic-App-Installer-v1.1-USB-MX4SIO.elf
  PSX_XMB_Apps/
    My Application/
      MyApp.ELF
      cover.png       optional
      app.ini         optional
```

A simple application folder needs exactly one ELF. Use `app.ini` to set a fixed ID, title, subtitle, or ELF filename:

```ini
id=my-application
title=My Application
subtitle=Homebrew Utility
elf=MyApp.ELF
```

Up to 16 application folders are scanned per run. Input ELF files must be 32-bit little-endian MIPS executables no larger than 2,025,312 bytes.

### Installation and updates

1. Copy the appropriate installer ELF and your `PSX_XMB_Apps` folder to the same medium.
2. Launch the ELF using wLaunchELF.
3. Check the detected revision and application list. Packages marked `ERROR` are skipped.
4. Hold `L1 + R1` and press `X`.
5. Wait for installation and verification to finish.
6. Fully power off the PSX, disconnect AC power, reconnect it, and boot XMB.

Keep an application's `id` unchanged when updating it. Running the installer again then updates its existing XMB entry. Removing its folder from the source medium does not uninstall it.

### Maintenance uninstall

Holding `L1 + R1 + L2 + R2` and pressing `TRIANGLE` opens a separate confirmation for removing **all** structurally verified manually installed XMB application partitions. This is not a single-app uninstall and should not be used for normal updates.

Source code, third-party notices, and SHA-256 checksums for both ELF files are included in the GitHub package.
