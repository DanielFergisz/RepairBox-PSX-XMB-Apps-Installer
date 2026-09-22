#!/usr/bin/env python3
"""Validate PSX_XMB_Apps packages before copying them to a USB drive."""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

ALLOWED_KEYS = {"id", "title", "subtitle", "elf"}
PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"


def normalize_id(value: str) -> str:
    output: list[str] = []
    separator = False
    for character in value:
        if character.isascii() and character.isalnum():
            output.append(character.lower())
            separator = False
        elif character in "-_ ":
            if output and not separator:
                output.append("-")
                separator = True
        else:
            raise ValueError("id accepts ASCII letters, digits, spaces, '-' and '_'")
    result = "".join(output).rstrip("-")
    if not result or len(result) >= 48:
        raise ValueError("normalized id must contain 1-47 characters")
    return result


def safe_text(value: str, maximum: int, field: str) -> str:
    if not value or len(value) > maximum or value.startswith("."):
        raise ValueError(f"{field} must contain 1-{maximum} characters")
    if any(ord(c) < 0x20 or ord(c) > 0x7E or c in "/\\:=" for c in value):
        raise ValueError(f"{field} contains an unsupported character")
    return value


def read_ini(path: Path) -> dict[str, str]:
    if not path.exists():
        return {}
    raw = path.read_bytes()
    if len(raw) >= 2048:
        raise ValueError("app.ini must be smaller than 2048 bytes")
    text = raw.decode("ascii")
    result: dict[str, str] = {}
    for number, source_line in enumerate(text.splitlines(), 1):
        line = source_line.strip()
        if not line or line.startswith(("#", ";")):
            continue
        if "=" not in line:
            raise ValueError(f"app.ini line {number} has no '='")
        key, value = (part.strip() for part in line.split("=", 1))
        key = key.lower()
        if key not in ALLOWED_KEYS:
            raise ValueError(f"unknown app.ini key: {key}")
        if not value:
            raise ValueError(f"empty app.ini value: {key}")
        result[key] = value
    return result


def validate_elf(path: Path) -> None:
    size = path.stat().st_size
    if size > 2_025_312:
        raise ValueError("ELF is larger than the installer limit (2,025,312 bytes)")
    header = path.read_bytes()[:20]
    if (len(header) != 20 or header[:4] != b"\x7fELF" or
            header[4:7] != b"\x01\x01\x01" or header[18:20] != b"\x08\x00"):
        raise ValueError("file is not a 32-bit little-endian MIPS ELF")


def validate_cover(path: Path) -> None:
    size = path.stat().st_size
    header = path.read_bytes()[:24]
    if size > 1024 * 1024:
        raise ValueError("cover.png is larger than 1 MiB")
    if len(header) != 24 or header[:8] != PNG_SIGNATURE or header[12:16] != b"IHDR":
        raise ValueError("cover.png is not a valid PNG")
    width, height = struct.unpack(">II", header[16:24])
    if not (16 <= width <= 256 and 16 <= height <= 256):
        raise ValueError("cover dimensions must be from 16x16 through 256x256")


def validate_package(folder: Path) -> tuple[str, str]:
    safe_text(folder.name, 31, "folder name")
    config = read_ini(folder / "app.ini")
    package_id = normalize_id(config.get("id", folder.name))
    safe_text(config.get("title", folder.name), 31, "title")
    if "subtitle" in config:
        safe_text(config["subtitle"], 31, "subtitle")
    elf_files = sorted(p for p in folder.iterdir()
                       if p.is_file() and p.suffix.lower() == ".elf")
    if "elf" in config:
        elf_name = safe_text(config["elf"], 63, "elf")
        if not elf_name.lower().endswith(".elf"):
            raise ValueError("configured elf does not end with .ELF")
        elf_path = folder / elf_name
    else:
        if len(elf_files) != 1:
            raise ValueError("folder without elf= must contain exactly one ELF")
        elf_path = elf_files[0]
    if not elf_path.is_file():
        raise ValueError(f"ELF not found: {elf_path.name}")
    validate_elf(elf_path)
    cover = folder / "cover.png"
    if cover.exists():
        validate_cover(cover)
    return package_id, elf_path.name


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("root", nargs="?", default="PSX_XMB_Apps")
    args = parser.parse_args()
    root = Path(args.root)
    if not root.is_dir():
        print(f"ERROR: directory not found: {root}")
        return 1
    folders = sorted(p for p in root.iterdir() if p.is_dir())
    if len(folders) > 16:
        print("ERROR: the PSX installer scans at most 16 folders")
        return 1
    ids: set[str] = set()
    failures = 0
    for folder in folders:
        try:
            package_id, elf_name = validate_package(folder)
            if package_id in ids:
                raise ValueError(f"duplicate normalized id: {package_id}")
            ids.add(package_id)
            print(f"PASS  {folder.name}  id={package_id}  elf={elf_name}")
        except (OSError, UnicodeError, ValueError) as error:
            failures += 1
            print(f"ERROR {folder.name}: {error}")
    print(f"Checked {len(folders)} package(s), failures: {failures}")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
