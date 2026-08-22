# RepairBox.pl PSX XMB Apps Installer v1.0

This installer adds a small set of PS2 homebrew apps to the Games menu on a
Sony PSX DESR. It works with both PSX1 and PSX2 systems and reads
`__system/version.txt` to detect the revision automatically.

It is made for a PSX that already has a working system. This is not an HDD
formatter or system installer.

## Included apps

| App | XMB partition |
| --- | --- |
| wLaunchELF R3Z/DS34 | `PP.APPS-00002..WLE` |
| Open PS2 Loader | `PP.APPS-00003..OPL` |
| PadTest | `PP.APPS-00004..PADTEST` |
| POPSLoader | `PP.APPS-00005..POPS` |

The full install and XMB launch were tested on both PSX1 and PSX2. Running the
installer again updates or repairs the same entries, so it will not add a
second copy of an app to the menu.

## What you need

- a working Sony PSX DESR system
- wLaunchELF v4.70_R3Z
- at least 512 MiB of unallocated HDD space
- the installer ELF and the complete `PSX_XMB_Apps` folder on USB

Your USB drive should look like this:

```text
mass:/
  RepairBox.pl-PSX-XMB-Apps-Installer-v1.0.elf
  PSX_XMB_Apps/
    WLE-R3Z-DS34/
    OPL/
    PADTEST/
    POPSLOADER/
```

Do not remove files from the app folders. Each one must contain
`EXECUTE.KELF`, `SYSTEM.CNF`, `icon.sys` and the three files under `res/`.

## Installing

1. Run the ELF from wLaunchELF v4.70_R3Z.
2. Check that all four packages show `PASS`.
3. Hold L1 + R1 and press X.
4. Wait for the installation to finish.
5. Turn the PSX off, disconnect AC power for a moment, then start it normally.

The apps should now appear in the Games section of XMB.

## Building

You need PS2DEV/PS2SDK, Python 3 and Make:

```sh
make clean all
```

The finished ELF is written to the project root. A prebuilt copy is also
available in `release/`.

## Updating an app

To replace one of the bundled apps without changing its XMB slot:

1. Replace its six-file package under `package/PSX_XMB_Apps/`.
2. Update its file sizes and SHA-256 hashes in `src/app_installer.c` and
   `tests/test_package.py`.
3. Keep the existing APA partition name.
4. Run `make clean all` and test the new ELF on real hardware.
