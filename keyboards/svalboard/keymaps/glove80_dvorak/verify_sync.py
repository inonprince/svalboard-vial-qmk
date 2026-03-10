#!/usr/bin/env python3

import json
import re
import sys
from pathlib import Path


HERE = Path(__file__).resolve().parent
KEYMAP_C = HERE / "keymap.c"
CONFIG_H = HERE / "config.h"
VIL = HERE.parent.parent / "vils" / "glove80_dvorak.vil"

LAYER_IDS = {
    "BASE": 0,
    "NAV": 1,
    "NUM": 2,
    "SYM": 3,
    "FUNC": 4,
    "SYS": 5,
    "BOARD_CONFIG": 14,
    "MBO": 15,
}

ALIASES = {
    "KC_APP": "KC_APP",
    "KC_APPLICATION": "KC_APP",
    "KC_BSPC": "KC_BSPACE",
    "KC_CAPS": "KC_CAPSLOCK",
    "KC_DEL": "KC_DELETE",
    "KC_EQL": "KC_EQUAL",
    "KC_ESC": "KC_ESCAPE",
    "KC_GRV": "KC_GRAVE",
    "KC_INS": "KC_INSERT",
    "KC_LBRC": "KC_LBRACKET",
    "KC_LCTL": "KC_LCTRL",
    "KC_LSFT": "KC_LSHIFT",
    "KC_MINS": "KC_MINUS",
    "KC_NUM": "KC_NUMLOCK",
    "KC_PGDN": "KC_PGDOWN",
    "KC_PSCR": "KC_PSCREEN",
    "KC_QUOT": "KC_QUOTE",
    "KC_RBRC": "KC_RBRACKET",
    "KC_SCLN": "KC_SCOLON",
    "KC_SCRL": "KC_SCROLLLOCK",
    "KC_SLSH": "KC_SLASH",
    "KC_BSLS": "KC_BSLASH",
    "KC_COMM": "KC_COMMA",
}

EXPECTED_SETTINGS = {
    "1": 0,
    "2": 50,
    "3": 0,
    "4": 175,
    "5": 5,
    "6": 5000,
    "9": 30,
    "10": 20,
    "11": 8,
    "12": 15,
    "13": 40,
    "14": 10,
    "15": 80,
    "16": 8,
    "17": 40,
    "18": 0,
    "19": 80,
    "20": 5,
    "21": 128,
    "22": 0,
    "23": 0,
    "24": 0,
    "26": 0,
    "27": 0,
}

INTENTIONAL_SOURCE_ONLY_DIFFS = {
    ("BASE", 5, 4): ("SV_APP_SWITCH", "KC_NO"),
}


def read_tapping_term() -> int:
    match = re.search(r"#define\s+TAPPING_TERM\s+(\d+)", CONFIG_H.read_text())
    if not match:
        raise SystemExit("Could not find TAPPING_TERM in config.h")
    return int(match.group(1))


def strip_comments(text: str) -> str:
    return re.sub(r"/\*.*?\*/", "", text, flags=re.S)


def read_simple_macros() -> dict[str, str]:
    macros = {}
    for line in KEYMAP_C.read_text().splitlines():
        match = re.match(r"#define\s+([A-Z0-9_]+)\s+(.+)$", line.strip())
        if not match:
            continue
        name, value = match.groups()
        if "(" in name:
            continue
        macros[name] = value.strip()
    return macros


def split_top_level(text: str) -> list[str]:
    parts = []
    current = []
    depth = 0
    for char in text:
        if char == "," and depth == 0:
            part = "".join(current).strip()
            if part:
                parts.append(part)
            current = []
            continue
        if char == "(":
            depth += 1
        elif char == ")":
            depth -= 1
        current.append(char)
    part = "".join(current).strip()
    if part:
        parts.append(part)
    return parts


def extract_layout_tokens(text: str, layer_name: str) -> list[str]:
    marker = f"[{layer_name}] = LAYOUT("
    start = text.find(marker)
    if start < 0:
        raise SystemExit(f"Could not find layer {layer_name} in keymap.c")
    pos = start + len(marker)
    depth = 1
    while pos < len(text) and depth:
        if text[pos] == "(":
            depth += 1
        elif text[pos] == ")":
            depth -= 1
        pos += 1
    body = strip_comments(text[start + len(marker) : pos - 1])
    tokens = split_top_level(body)
    if len(tokens) != 60:
        raise SystemExit(f"Expected 60 tokens for {layer_name}, found {len(tokens)}")
    return tokens


def finger_to_matrix(tokens: list[str]) -> list[str]:
    center, north, east, south, west, double = tokens
    return [south, east, center, north, west, double]


def thumb_to_matrix(tokens: list[str]) -> list[str]:
    down, pad, up, nail, knuckle, double = tokens
    return [knuckle, nail, down, pad, up, double]


def source_layer_to_matrix_rows(tokens: list[str]) -> list[list[str]]:
    rows = [tokens[i : i + 6] for i in range(0, len(tokens), 6)]
    r1, r2, r3, r4, l1, l2, l3, l4, rt, lt = rows
    return [
        thumb_to_matrix(lt),
        finger_to_matrix(l1),
        finger_to_matrix(l2),
        finger_to_matrix(l3),
        finger_to_matrix(l4),
        thumb_to_matrix(rt),
        finger_to_matrix(r1),
        finger_to_matrix(r2),
        finger_to_matrix(r3),
        finger_to_matrix(r4),
    ]


def normalize(token, macros: dict[str, str]) -> str:
    if token == -1 or token == "-1":
        return "KC_NO"
    value = re.sub(r"\s+", "", str(token))
    if value in macros:
        return normalize(macros[value], macros)
    value = re.sub(r"LGUI\(LSFT\((.+)\)\)", r"SGUI(\1)", value)
    value = re.sub(r"LSFT\(LGUI\((.+)\)\)", r"SGUI(\1)", value)
    value = re.sub(r"LALT\(LSFT\((.+)\)\)", r"LSA(\1)", value)
    value = re.sub(r"LSFT\(LALT\((.+)\)\)", r"LSA(\1)", value)
    value = value.replace("S(", "LSFT(")
    for layer_name, layer_id in LAYER_IDS.items():
        value = re.sub(rf"LT\({layer_name},", f"LT({layer_id},", value)
    for old, new in ALIASES.items():
        value = re.sub(rf"\b{re.escape(old)}\b", new, value)
    return value


def compare_layers() -> list[str]:
    keymap_text = KEYMAP_C.read_text()
    vil = json.loads(VIL.read_text())
    macros = read_simple_macros()
    errors = []
    for layer_name, layer_id in LAYER_IDS.items():
        expected = source_layer_to_matrix_rows(extract_layout_tokens(keymap_text, layer_name))
        actual = vil["layout"][layer_id]
        for row_idx, (exp_row, act_row) in enumerate(zip(expected, actual)):
            for col_idx, (exp, act) in enumerate(zip(exp_row, act_row)):
                norm_exp = normalize(exp, macros)
                norm_act = normalize(act, macros)
                if norm_exp != norm_act:
                    allowed = INTENTIONAL_SOURCE_ONLY_DIFFS.get((layer_name, row_idx, col_idx))
                    if allowed == (norm_exp, norm_act):
                        continue
                    errors.append(
                        f"Layer {layer_name} ({layer_id}) row {row_idx} col {col_idx}: "
                        f"source={norm_exp} vil={norm_act}"
                    )
    return errors


def compare_settings() -> list[str]:
    vil = json.loads(VIL.read_text())
    settings = {str(k): v for k, v in vil.get("settings", {}).items()}
    tapping_term = read_tapping_term()
    expected = {
        "7": tapping_term,
        "25": tapping_term,
        **EXPECTED_SETTINGS,
    }
    errors = []
    unknown = sorted(set(settings) - set(expected), key=int)
    if unknown:
        errors.append(f"Unexpected setting ids present: {', '.join(unknown)}")
    for key, value in expected.items():
        actual = settings.get(key)
        if actual != value:
            errors.append(f"Setting {key}: expected {value}, found {actual}")
    return errors


def validate_vil_layout_encoding() -> list[str]:
    vil = json.loads(VIL.read_text())
    errors = []
    for layer_idx, layer in enumerate(vil.get("layout", [])):
        for row_idx, row in enumerate(layer):
            for col_idx, value in enumerate(row):
                if value == -1:
                    errors.append(
                        f"Layout layer {layer_idx} row {row_idx} col {col_idx}: "
                        "found numeric -1, expected KC_NO"
                    )
    if vil.get("layout_options") != 0:
        errors.append(f"layout_options: expected 0, found {vil.get('layout_options')}")
    if len(vil.get("alt_repeat_key", [])) != 32:
        errors.append(
            f"alt_repeat_key length: expected 32, found {len(vil.get('alt_repeat_key', []))}"
        )
    if len(vil.get("key_override", [])) != 30:
        errors.append(
            f"key_override length: expected 30, found {len(vil.get('key_override', []))}"
        )
    return errors


def main() -> int:
    errors = compare_layers() + compare_settings() + validate_vil_layout_encoding()
    if errors:
        for error in errors:
            print(error)
        return 1
    print("glove80_dvorak source keymap and .vil are in sync")
    return 0


if __name__ == "__main__":
    sys.exit(main())
