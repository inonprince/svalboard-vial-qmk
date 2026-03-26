#!/usr/bin/env python3
"""Flash a UF2 firmware file to an RP2040 board.

Waits for the RP2040 bootloader drive to appear, then copies the firmware file to it.
"""

import argparse
import shutil
import sys
import termios
import time
import tty
from pathlib import Path

BOOTLOADER_VOLUME = "RPI-RP2"
POLL_INTERVAL_SECONDS = 0.5


def find_rp2040_drive():
    """Return the path to the RP2040 bootloader drive if mounted, else None."""
    volume = Path("/Volumes") / BOOTLOADER_VOLUME
    if volume.is_dir():
        return volume
    return None


def wait_for_drive():
    """Poll until the RP2040 bootloader drive appears and return its path."""
    print(f"Waiting for {BOOTLOADER_VOLUME} drive...")
    print("Put your board into bootloader mode now.")
    while True:
        drive = find_rp2040_drive()
        if drive is not None:
            return drive
        time.sleep(POLL_INTERVAL_SECONDS)


def find_newest_uf2(directory):
    """Return the most recently modified .uf2 file in the directory."""
    uf2_files = sorted(directory.glob("*.uf2"), key=lambda p: p.stat().st_mtime, reverse=True)
    if not uf2_files:
        sys.exit(f"Error: no .uf2 files found in {directory}")
    return uf2_files[0]


def read_single_key():
    """Read a single keypress without waiting for Enter."""
    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    try:
        tty.setraw(fd)
        return sys.stdin.read(1)
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)


def confirm_file(firmware_path):
    """Ask the user to confirm the selected file with a single keypress."""
    print(f"Selected: \033[1;32m{firmware_path.name}\033[0m")
    print("Flash this file? [Y/n] ", end="", flush=True)
    key = read_single_key().lower()
    print(key)
    if key not in ("y", "\r", "\n"):
        sys.exit("Aborted.")


def flash(firmware_path, drive_path):
    """Copy the UF2 file to the bootloader drive."""
    dest = drive_path / firmware_path.name
    print(f"Copying {firmware_path.name} to {drive_path} ...")
    shutil.copy2(firmware_path, dest)
    print("Done! The board should reboot automatically.")


def main():
    parser = argparse.ArgumentParser(description="Flash a UF2 firmware file to an RP2040 board.")
    parser.add_argument("firmware", type=Path, help="Path to a .uf2 file or a directory containing .uf2 files")
    args = parser.parse_args()

    firmware = args.firmware.resolve()

    if not firmware.exists():
        sys.exit(f"Error: path not found: {firmware}")

    if firmware.is_dir():
        firmware = find_newest_uf2(firmware)
        confirm_file(firmware)
    elif firmware.suffix.lower() != ".uf2":
        sys.exit(f"Error: expected a .uf2 file, got {firmware.suffix}")

    existing_drive = find_rp2040_drive()
    if existing_drive:
        print(f"Found {BOOTLOADER_VOLUME} already mounted.")
        flash(firmware, existing_drive)
        return

    drive = wait_for_drive()
    print(f"Detected {BOOTLOADER_VOLUME} at {drive}")
    flash(firmware, drive)


if __name__ == "__main__":
    main()
