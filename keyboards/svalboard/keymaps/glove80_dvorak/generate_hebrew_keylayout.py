#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import pathlib
import re
import shutil
import sys
from dataclasses import dataclass

from render_kle import (
    KEYMAP_PATH,
    build_layer_slot_map,
    parse_call,
    resolve_alias,
)


SCRIPT_DIR = pathlib.Path(__file__).resolve().parent
POSITIONS_PATH = SCRIPT_DIR / "hebrew_qwerty_positions.json"
KEYLAYOUT_RELATIVE_PATH = pathlib.Path(
    "Contents/Resources/Hebrew Dvorak.keylayout"
)

DEFAULT_BUNDLE_CANDIDATES = [
    pathlib.Path.home() / "Downloads/Hebrew-Dvorak-22-01-25/Hebrew Dvorak.bundle",
    pathlib.Path("/Library/Keyboard Layouts/Hebrew Dvorak.bundle"),
    SCRIPT_DIR / "Hebrew-Dvorak-22-01-25" / "Hebrew Dvorak.bundle",
]

# Apple virtual key codes for an ANSI keyboard. QMK sends these logical keys to
# macOS; the .keylayout then turns the virtual key code into text.
QMK_TO_MAC_CODE = {
    "KC_A": 0,
    "KC_S": 1,
    "KC_D": 2,
    "KC_F": 3,
    "KC_H": 4,
    "KC_G": 5,
    "KC_Z": 6,
    "KC_X": 7,
    "KC_C": 8,
    "KC_V": 9,
    "KC_B": 11,
    "KC_Q": 12,
    "KC_W": 13,
    "KC_E": 14,
    "KC_R": 15,
    "KC_Y": 16,
    "KC_T": 17,
    "KC_1": 18,
    "KC_2": 19,
    "KC_3": 20,
    "KC_4": 21,
    "KC_6": 22,
    "KC_5": 23,
    "KC_EQL": 24,
    "KC_9": 25,
    "KC_7": 26,
    "KC_MINS": 27,
    "KC_8": 28,
    "KC_0": 29,
    "KC_RBRC": 30,
    "KC_O": 31,
    "KC_U": 32,
    "KC_LBRC": 33,
    "KC_I": 34,
    "KC_P": 35,
    "KC_L": 37,
    "KC_J": 38,
    "KC_QUOT": 39,
    "KC_K": 40,
    "KC_SCLN": 41,
    "KC_BSLS": 42,
    "KC_COMM": 43,
    "KC_SLSH": 44,
    "KC_N": 45,
    "KC_M": 46,
    "KC_DOT": 47,
    "KC_GRV": 50,
}

QMK_ALIASES = {
    "KC_EQUAL": "KC_EQL",
    "KC_MINUS": "KC_MINS",
    "KC_LBRACKET": "KC_LBRC",
    "KC_RBRACKET": "KC_RBRC",
    "KC_BSLASH": "KC_BSLS",
    "KC_SCOLON": "KC_SCLN",
    "KC_SQUOTE": "KC_QUOT",
    "KC_QUOTE": "KC_QUOT",
    "KC_COMMA": "KC_COMM",
    "KC_SLASH": "KC_SLSH",
    "KC_GRAVE": "KC_GRV",
}

# Classic Hebrew-on-QWERTY output. Entries such as '"' and ':' model Svalboard
# keys that are dedicated shifted-symbol slots in the QWERTY approximation.
HEBREW_QWERTY = {
    "`": "׳",
    "1": "1",
    "2": "2",
    "3": "3",
    "4": "4",
    "5": "5",
    "6": "6",
    "7": "7",
    "8": "8",
    "9": "9",
    "0": "0",
    "-": "-",
    "=": "=",
    "Q": "/",
    "W": "'",
    "E": "ק",
    "R": "ר",
    "T": "א",
    "Y": "ט",
    "U": "ו",
    "I": "ן",
    "O": "ם",
    "P": "פ",
    "[": "]",
    "]": "[",
    "\\": "\\",
    "A": "ש",
    "S": "ד",
    "D": "ג",
    "F": "כ",
    "G": "ע",
    "H": "י",
    "J": "ח",
    "K": "ל",
    "L": "ך",
    ";": "ף",
    "'": ",",
    "Z": "ז",
    "X": "ס",
    "C": "ב",
    "V": "ה",
    "B": "נ",
    "N": "מ",
    "M": "צ",
    ",": "ת",
    ".": "ץ",
    "/": ".",
    "~": "~",
    "!": "!",
    "@": "@",
    "#": "#",
    "$": "$",
    "%": "%",
    "^": "^",
    "&": "&",
    "*": "*",
    "(": ")",
    ")": "(",
    "_": "_",
    "+": "+",
    "{": "{",
    "}": "}",
    "|": "|",
    ":": ":",
    "\"": "״",
    "<": ">",
    ">": "<",
    "?": "?",
}

NON_TEXT_QWERTY_SLOTS = {
    "DEL",
    "ESC",
    "WIN",
}


@dataclass(frozen=True)
class KeyEvent:
    qmk_key: str
    shifted: bool

    @property
    def keymap_index(self) -> int:
        return 3 if self.shifted else 2


@dataclass(frozen=True)
class LayoutUpdate:
    position: str
    qwerty_slot: str
    hebrew_output: str
    source_token: str
    event: KeyEvent
    mac_code: int


def load_positions(path: pathlib.Path) -> dict[str, str]:
    data = json.loads(path.read_text())
    positions = data.get("positions", data)
    if not isinstance(positions, dict):
        raise ValueError(f"{path} must contain an object or a positions object")
    return {str(key).lower(): normalize_qwerty_slot(str(value)) for key, value in positions.items()}


def normalize_qwerty_slot(raw: str) -> str:
    aliases = {
        "GRV": "`",
        "GRAVE": "`",
        "TILDE": "~",
        "MINUS": "-",
        "MINS": "-",
        "EQL": "=",
        "EQUAL": "=",
        "LBRC": "[",
        "LBRACKET": "[",
        "RBRC": "]",
        "RBRACKET": "]",
        "BSLS": "\\",
        "BACKSLASH": "\\",
        "SCLN": ";",
        "SEMICOLON": ";",
        "COLON": ":",
        "DEL": "DEL",
        "DELETE": "DEL",
        "ESC": "ESC",
        "ESCAPE": "ESC",
        "WIN": "WIN",
        "GUI": "WIN",
        "CMD": "WIN",
        "COMMAND": "WIN",
        "QUOT": "'",
        "QUOTE": "'",
        "APOSTROPHE": "'",
        "DQUO": "\"",
        "DOUBLE_QUOTE": "\"",
        "COMM": ",",
        "COMMA": ",",
        "DOT": ".",
        "PERIOD": ".",
        "SLSH": "/",
        "SLASH": "/",
    }
    key = raw.strip()
    upper = key.upper()
    if upper in aliases:
        return aliases[upper]
    if len(upper) == 1 and upper in "ABCDEFGHIJKLMNOPQRSTUVWXYZ":
        return upper
    if len(key) == 1 and key in HEBREW_QWERTY:
        return key
    if upper in NON_TEXT_QWERTY_SLOTS:
        return upper
    raise ValueError(f"Unsupported QWERTY slot {raw!r}")


def canonical_qmk_key(raw: str) -> str:
    key = raw.strip()
    key = QMK_ALIASES.get(key, key)
    if len(key) == 4 and key.startswith("KC_") and key[-1].isalpha():
        key = key.upper()
    return key


def resolve_key_event(token: str, defines: dict[str, str], shifted: bool = False) -> KeyEvent | None:
    token = resolve_alias(token, defines)
    call = parse_call(token)
    if call is not None:
        func, args = call
        if func in {"S", "LSFT", "RSFT"} and len(args) == 1:
            return resolve_key_event(args[0], defines, True)
        if func in {"LCTL_T", "RCTL_T", "LALT_T", "RALT_T", "LGUI_T", "RGUI_T", "LSFT_T", "RSFT_T"} and len(args) == 1:
            return resolve_key_event(args[0], defines, shifted)
        if func == "LT" and len(args) == 2:
            return resolve_key_event(args[1], defines, shifted)
        return None

    qmk_key = canonical_qmk_key(token)
    if qmk_key not in QMK_TO_MAC_CODE:
        return None
    return KeyEvent(qmk_key=qmk_key, shifted=shifted)


def build_updates(
    keymap_path: pathlib.Path,
    positions_path: pathlib.Path,
    dynamic_layer_count: int,
) -> list[LayoutUpdate]:
    layer_slot_map, _layers, defines, _slot_labels = build_layer_slot_map(
        keymap_path,
        dynamic_layer_count,
    )
    base = layer_slot_map["BASE"]
    positions = load_positions(positions_path)

    updates: list[LayoutUpdate] = []
    for position, qwerty_slot in positions.items():
        if position not in base:
            raise ValueError(f"{positions_path}: unknown Svalboard position {position!r}")
        if qwerty_slot in NON_TEXT_QWERTY_SLOTS:
            continue
        if qwerty_slot not in HEBREW_QWERTY:
            raise ValueError(f"No Hebrew QWERTY output is defined for {qwerty_slot!r}")

        source_token = base[position]
        event = resolve_key_event(source_token, defines)
        if event is None:
            continue

        updates.append(
            LayoutUpdate(
                position=position,
                qwerty_slot=qwerty_slot,
                hebrew_output=HEBREW_QWERTY[qwerty_slot],
                source_token=source_token,
                event=event,
                mac_code=QMK_TO_MAC_CODE[event.qmk_key],
            )
        )

    updates.sort(key=lambda update: (update.position, update.event.keymap_index, update.mac_code))
    validate_no_conflicts(updates)
    return updates


def validate_no_conflicts(updates: list[LayoutUpdate]) -> None:
    seen: dict[tuple[int, int], LayoutUpdate] = {}
    for update in updates:
        key = (update.event.keymap_index, update.mac_code)
        previous = seen.get(key)
        if previous is None:
            seen[key] = update
            continue
        if previous.hebrew_output != update.hebrew_output:
            raise ValueError(
                "Conflicting generated output for keyMap "
                f"{key[0]} code {key[1]}: "
                f"{previous.position}->{previous.hebrew_output!r} and "
                f"{update.position}->{update.hebrew_output!r}"
            )


def xml_escape_attr(value: str) -> str:
    return (
        value.replace("&", "&amp;")
        .replace("\"", "&quot;")
        .replace("<", "&lt;")
        .replace(">", "&gt;")
    )


def update_keylayout_text(text: str, updates: list[LayoutUpdate]) -> str:
    by_keymap: dict[int, list[LayoutUpdate]] = {}
    for update in updates:
        by_keymap.setdefault(update.event.keymap_index, []).append(update)

    for keymap_index, keymap_updates in sorted(by_keymap.items()):
        text = update_keymap_block(text, keymap_index, keymap_updates)
    return text


def update_keymap_block(text: str, keymap_index: int, updates: list[LayoutUpdate]) -> str:
    block_pattern = re.compile(
        rf"(?P<head><keyMap\s+index=\"{keymap_index}\">\n)"
        rf"(?P<body>.*?)"
        rf"(?P<tail>\s*</keyMap>)",
        flags=re.S,
    )
    block_match = block_pattern.search(text)
    if not block_match:
        raise ValueError(f"Could not find <keyMap index=\"{keymap_index}\">")

    body = block_match.group("body")
    for update in updates:
        key_pattern = re.compile(
            rf"(?P<head><key\s+code=\"{update.mac_code}\"\s+output=\")"
            rf"(?P<output>[^\"]*)"
            rf"(?P<tail>\"\s*/>)"
        )
        replacement = (
            lambda match, output=xml_escape_attr(update.hebrew_output): (
                match.group("head") + output + match.group("tail")
            )
        )
        body, count = key_pattern.subn(replacement, body, count=1)
        if count != 1:
            raise ValueError(
                f"Could not find key code {update.mac_code} in keyMap {keymap_index}"
            )

    return text[: block_match.start("body")] + body + text[block_match.end("body") :]


def default_bundle_path() -> pathlib.Path | None:
    for candidate in DEFAULT_BUNDLE_CANDIDATES:
        if candidate.exists():
            return candidate
    return None


def resolve_keylayout_path(bundle_path: pathlib.Path) -> pathlib.Path:
    keylayout = bundle_path / KEYLAYOUT_RELATIVE_PATH
    if not keylayout.exists():
        raise ValueError(f"Could not find keylayout file at {keylayout}")
    return keylayout


def print_report(updates: list[LayoutUpdate]) -> None:
    columns = [
        ("position", 8),
        ("qwerty", 8),
        ("hebrew", 8),
        ("token", 16),
        ("sent", 10),
        ("map", 5),
        ("code", 4),
    ]
    header = " ".join(name.ljust(width) for name, width in columns)
    print(header.rstrip())
    print(" ".join("-" * width for _name, width in columns).rstrip())
    for update in updates:
        sent = ("S+" if update.event.shifted else "") + update.event.qmk_key.removeprefix("KC_")
        row = [
            update.position,
            update.qwerty_slot,
            update.hebrew_output,
            update.source_token,
            sent,
            str(update.event.keymap_index),
            str(update.mac_code),
        ]
        print(" ".join(value.ljust(width) for value, (_name, width) in zip(row, columns)).rstrip())


def copy_bundle(source: pathlib.Path, destination: pathlib.Path, force: bool) -> pathlib.Path:
    if destination.exists():
        if not force:
            raise ValueError(f"{destination} already exists; pass --force to replace it")
        shutil.rmtree(destination)
    shutil.copytree(source, destination)
    return destination


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Generate the Hebrew macOS keylayout mapping from the Svalboard BASE layer."
    )
    parser.add_argument(
        "--keymap",
        type=pathlib.Path,
        default=KEYMAP_PATH,
        help=f"QMK keymap source to parse. Default: {KEYMAP_PATH}",
    )
    parser.add_argument(
        "--positions",
        type=pathlib.Path,
        default=POSITIONS_PATH,
        help=f"Svalboard-position to QWERTY-slot JSON. Default: {POSITIONS_PATH}",
    )
    parser.add_argument(
        "--bundle",
        type=pathlib.Path,
        default=None,
        help="Source Hebrew Dvorak.bundle. Defaults to the first known existing bundle path.",
    )
    parser.add_argument(
        "--output-bundle",
        type=pathlib.Path,
        default=None,
        help="Copy --bundle here and update the copied keylayout.",
    )
    parser.add_argument(
        "--in-place",
        action="store_true",
        help="Update --bundle directly instead of only printing the generated table.",
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="Allow --output-bundle to replace an existing bundle directory.",
    )
    parser.add_argument(
        "--dynamic-layer-count",
        type=int,
        default=16,
        help="Value to use for DYNAMIC_KEYMAP_LAYER_COUNT when resolving layer expressions.",
    )
    parser.add_argument(
        "--quiet",
        action="store_true",
        help="Do not print the generated mapping table.",
    )
    args = parser.parse_args()

    if args.in_place and args.output_bundle is not None:
        raise ValueError("Use either --in-place or --output-bundle, not both")

    updates = build_updates(args.keymap, args.positions, args.dynamic_layer_count)
    if not args.quiet:
        print_report(updates)

    if not args.in_place and args.output_bundle is None:
        print(f"\nDry run only: {len(updates)} keylayout entries generated.")
        return 0

    bundle = args.bundle or default_bundle_path()
    if bundle is None:
        raise ValueError("No bundle found; pass --bundle PATH")
    bundle = bundle.expanduser()

    target_bundle = bundle
    if args.output_bundle is not None:
        target_bundle = copy_bundle(bundle, args.output_bundle.expanduser(), args.force)

    keylayout_path = resolve_keylayout_path(target_bundle)
    original = keylayout_path.read_text()
    updated = update_keylayout_text(original, updates)
    keylayout_path.write_text(updated)
    print(f"\nUpdated {keylayout_path}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        raise SystemExit(1)
