#!/usr/bin/env python3
"""Flash a UF2 firmware file to an RP2040 board.

Waits for the RP2040 bootloader drive to appear, then copies the firmware file to it.
"""

import argparse
import re
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


def _match_case(source, target):
    """Apply the case pattern of source to target."""
    if source.islower():
        return target.lower()
    if source.isupper():
        return target.upper()
    if source[0].isupper():
        return target.capitalize()
    return target.lower()


def detect_side(filename):
    """Detect if a filename contains a 'left' or 'right' side indicator."""
    name_lower = filename.lower()
    if "left" in name_lower:
        return "left"
    if "right" in name_lower:
        return "right"
    return None


def find_paired_file(firmware_path):
    """Find the other-side file matching this firmware.

    Returns (left_path, right_path) if a pair exists, else None.
    """
    name = firmware_path.name
    side = detect_side(name)
    if side is None:
        return None

    if side == "left":
        other_name = re.sub(r"(?i)left", lambda m: _match_case(m.group(), "right"), name)
    else:
        other_name = re.sub(r"(?i)right", lambda m: _match_case(m.group(), "left"), name)

    other_path = firmware_path.parent / other_name
    if not other_path.exists():
        return None

    if side == "left":
        return firmware_path, other_path
    return other_path, firmware_path


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


def wait_for_disconnect():
    """Wait for the RP2040 bootloader drive to disappear."""
    print("Disconnect the keyboard...")
    while find_rp2040_drive() is not None:
        time.sleep(POLL_INTERVAL_SECONDS)
    time.sleep(1)
    print("Put the other side into bootloader mode now.")
    time.sleep(1)


def wait_and_flash(firmware_path):
    """Acquire the bootloader drive and flash the firmware."""
    existing_drive = find_rp2040_drive()
    if existing_drive:
        print(f"Found {BOOTLOADER_VOLUME} already mounted.")
        flash(firmware_path, existing_drive)
    else:
        drive = wait_for_drive()
        print(f"Detected {BOOTLOADER_VOLUME} at {drive}")
        time.sleep(1)
        flash(firmware_path, drive)


def main():
    parser = argparse.ArgumentParser(description="Flash a UF2 firmware file to an RP2040 board.")
    parser.add_argument("firmware", type=Path, help="Path to a .uf2 file or a directory containing .uf2 files")
    args = parser.parse_args()

    firmware = args.firmware.resolve()

    if not firmware.exists():
        sys.exit(f"Error: path not found: {firmware}")

    if firmware.is_dir():
        firmware = find_newest_uf2(firmware)
        pair = find_paired_file(firmware)
        if pair:
            left_path, right_path = pair

            print(f"Left side:  \033[1;32m{left_path.name}\033[0m")
            print("Flash left side? [Y/n] ", end="", flush=True)
            key = read_single_key().lower()
            print(key)
            flash_left = key in ("y", "\r", "\n")

            print(f"Right side: \033[1;32m{right_path.name}\033[0m")
            print("Flash right side? [Y/n] ", end="", flush=True)
            key = read_single_key().lower()
            print(key)
            flash_right = key in ("y", "\r", "\n")

            if not flash_left and not flash_right:
                sys.exit("Aborted.")
            if flash_left:
                wait_and_flash(left_path)
            if flash_right:
                if flash_left:
                    wait_for_disconnect()
                wait_and_flash(right_path)
            return
        confirm_file(firmware)
    elif firmware.suffix.lower() != ".uf2":
        sys.exit(f"Error: expected a .uf2 file, got {firmware.suffix}")

    wait_and_flash(firmware)


if __name__ == "__main__":
    main()
