#!/usr/bin/env python3
from __future__ import annotations

import argparse
import ast
import json
import pathlib
import re
import sys
from typing import Iterable


REPO_ROOT = pathlib.Path(__file__).resolve().parents[4]
KEYMAP_PATH = pathlib.Path(__file__).resolve().with_name("keymap.c")
VIAL_PATH = pathlib.Path(__file__).resolve().with_name("vial.json")
INFO_PATH = REPO_ROOT / "keyboards" / "svalboard" / "info.json"
KEYMAP_SUPPORT_PATH = REPO_ROOT / "keyboards" / "svalboard" / "keymaps" / "keymap_support.h"

# This matches the 52 real physical keys in the Svalboard KLE template. The
# decorative Trackpoint tile is intentionally omitted.
KLE_PHYSICAL_ORDER = [
    "l3n", "l2n", "r2n", "r3n",
    "l3w", "l3c", "l3e", "l2w", "l2c", "l2e", "r2w", "r2c", "r2e", "r3w", "r3c", "r3e",
    "l4n", "l1n", "r1n", "r4n",
    "l3s", "l2s", "r2s", "r3s",
    "l4w", "l4c", "l4e", "l1w", "l1c", "l1e", "r1w", "r1c", "r1e", "r4w", "r4c", "r4e",
    "l4s", "l1s", "r1s", "r4s",
    "lti", "ltdd", "ltuo", "rtuo", "rtdd", "rti",
    "ltu", "rtu",
    "ltd", "ltlo", "rtlo", "rtd",
]

BASE_HOME_ROW_MOD_SLOTS = {
    "l4c",
    "l3c",
    "l2c",
    "l1c",
    "r1c",
    "r2c",
    "r3c",
    "r4c",
}

LEGEND_COLORS = [
    "#ffffff",
    "#3b64ff",
    "#ff3b3b",
    "#f9ff00",
    "#4a5eff",
    "#5fffb0",
]

LEGEND_FONT_SIZES = [4, 2, 2, 2, 1, 1]
# KLE alignment 4 ("center front") gives us the four visible corner legend
# slots we want for BASE/NAV/NUM/SYM. Some thumb keys in the template use
# alignments like 5 or 7, which do not expose a visible third legend slot.
LEGEND_ALIGNMENT = 4

KEY_LABELS = {
    "KC_A": "A",
    "KC_B": "B",
    "KC_C": "C",
    "KC_D": "D",
    "KC_E": "E",
    "KC_F": "F",
    "KC_G": "G",
    "KC_H": "H",
    "KC_I": "I",
    "KC_J": "J",
    "KC_K": "K",
    "KC_L": "L",
    "KC_M": "M",
    "KC_N": "N",
    "KC_O": "O",
    "KC_P": "P",
    "KC_Q": "Q",
    "KC_R": "R",
    "KC_S": "S",
    "KC_T": "T",
    "KC_U": "U",
    "KC_V": "V",
    "KC_W": "W",
    "KC_X": "X",
    "KC_Y": "Y",
    "KC_Z": "Z",
    "KC_0": "0",
    "KC_1": "1",
    "KC_2": "2",
    "KC_3": "3",
    "KC_4": "4",
    "KC_5": "5",
    "KC_6": "6",
    "KC_7": "7",
    "KC_8": "8",
    "KC_9": "9",
    "KC_GRAVE": "`",
    "KC_GRV": "`",
    "GRV": "`",
    "KC_QUOTE": "'",
    "KC_QUOT": "'",
    "QUOT": "'",
    "KC_DQUO": "\"",
    "KC_COMM": ",",
    "KC_DOT": ".",
    "KC_SLSH": "/",
    "KC_BSLS": "\\",
    "KC_SCLN": ";",
    "KC_SCOLON": ";",
    "KC_COLON": ":",
    "KC_MINS": "-",
    "KC_MINUS": "-",
    "KC_EQL": "=",
    "KC_EQUAL": "=",
    "KC_LBRC": "[",
    "KC_RBRC": "]",
    "KC_LEFT": "←",
    "KC_RIGHT": "→",
    "KC_UP": "↑",
    "KC_DOWN": "↓",
    "KC_HOME": "home",
    "KC_END": "end",
    "KC_PGUP": "pgup",
    "KC_PGDN": "pgdn",
    "KC_INS": "ins",
    "KC_INSERT": "ins",
    "KC_DEL": "del",
    "KC_DELETE": "del",
    "KC_BSPC": "bksp",
    "KC_BSPACE": "bksp",
    "KC_ESC": "esc",
    "KC_ESCAPE": "esc",
    "KC_TAB": "tab",
    "KC_ENTER": "ent",
    "KC_ENT": "ent",
    "KC_SPACE": "spc",
    "KC_SPC": "spc",
    "KC_LCTL": "⌃",
    "KC_RCTL": "⌃",
    "KC_LALT": "⌥",
    "KC_RALT": "⌥",
    "KC_LGUI": "⌘",
    "KC_RGUI": "⌘",
    "KC_LSFT": "⇧",
    "KC_RSFT": "⇧",
    "KC_CAPS": "caps",
    "KC_CAPSLOCK": "caps",
    "KC_NUM": "numlk",
    "KC_NUMLOCK": "numlk",
    "KC_SCRL": "scrlk",
    "KC_SCROLLLOCK": "scrlk",
    "KC_PAUSE": "pause",
    "KC_APP": "menu",
    "KC_PSCR": "prtsc",
    "KC_PSCREEN": "prtsc",
    "KC_WHOM": "home",
    "KC_CALC": "calc",
    "KC_EJCT": "ejct",
    "KC_MPLY": "play",
    "KC_MPRV": "prev",
    "KC_MNXT": "next",
    "KC_MUTE": "mute",
    "KC_VOLU": "vol+",
    "KC_VOLD": "vol-",
    "KC_BRIU": "bri+",
    "KC_BRID": "bri-",
    "KC_BTN1": "btn1",
    "KC_BTN2": "btn2",
    "KC_BTN3": "btn3",
    "KC_TRNS": "",
    "KC_NO": "",
}

SHIFTED_KEY_LABELS = {
    "KC_1": "!",
    "KC_2": "@",
    "KC_3": "#",
    "KC_4": "$",
    "KC_5": "%",
    "KC_6": "^",
    "KC_7": "&",
    "KC_8": "*",
    "KC_9": "(",
    "KC_0": ")",
    "KC_GRAVE": "~",
    "KC_GRV": "~",
    "GRV": "~",
    "KC_MINS": "_",
    "KC_MINUS": "_",
    "KC_EQL": "+",
    "KC_EQUAL": "+",
    "KC_LBRC": "{",
    "KC_RBRC": "}",
    "KC_BSLS": "|",
    "KC_SCLN": ":",
    "KC_SCOLON": ":",
    "KC_QUOTE": "\"",
    "KC_QUOT": "\"",
    "QUOT": "\"",
    "KC_COMM": "<",
    "KC_DOT": ">",
    "KC_SLSH": "?",
}

MOD_TAP_NAMES = {
    "LCTL_T": "⌃",
    "RCTL_T": "⌃",
    "LALT_T": "⌥",
    "RALT_T": "⌥",
    "LGUI_T": "⌘",
    "RGUI_T": "⌘",
    "LSFT_T": "⇧",
    "RSFT_T": "⇧",
}

MOD_PREFIXES = {
    "LGUI": ["⌘"],
    "RGUI": ["⌘"],
    "LALT": ["⌥"],
    "RALT": ["⌥"],
    "LCTL": ["⌃"],
    "RCTL": ["⌃"],
    "LSFT": ["⇧"],
    "RSFT": ["⇧"],
    "SGUI": ["⇧", "⌘"],
    "LSA": ["⇧", "⌥"],
    "LCA": ["⌃", "⌥"],
    "LCAG": ["⌃", "⌥", "⌘"],
    "HYPR": ["⌃", "⇧", "⌥", "⌘"],
    "MEH": ["⌃", "⇧", "⌥"],
}

OSM_MOD_LABELS = {
    "MOD_LSFT": "1s⇧",
    "MOD_RSFT": "1s⇧",
    "MOD_LCTL": "1s⌃",
    "MOD_RCTL": "1s⌃",
    "MOD_LALT": "1s⌥",
    "MOD_RALT": "1s⌥",
    "MOD_LGUI": "1s⌘",
    "MOD_RGUI": "1s⌘",
}

CUSTOM_LABEL_OVERRIDES = {
    "SV_LEFT_DPI_INC": "dpi+L",
    "SV_LEFT_DPI_DEC": "dpi-L",
    "SV_RIGHT_DPI_INC": "dpi+R",
    "SV_RIGHT_DPI_DEC": "dpi-R",
    "SV_LEFT_SCROLL_TOGGLE": "scrl L",
    "SV_RIGHT_SCROLL_TOGGLE": "scrl R",
    "SV_RECALIBRATE_POINTER": "recal",
    "SV_MH_CHANGE_TIMEOUTS": "ms tmr",
    "SV_CAPS_WORD": "capswd",
    "SV_AXIS_SCROLL_LOCK": "axislk",
    "SV_SNIPER_2": "snip2",
    "SV_SNIPER_3": "snip3",
    "SV_SNIPER_5": "snip5",
    "SV_SCROLL_HOLD": "scrhold",
    "SV_SCROLL_TOGGLE": "scrtog",
    "SV_OUTPUT_STATUS": "status",
    "SV_TOGGLE_AUTOMOUSE": "auto ms",
    "SV_TURBO_SCAN": "turbo",
    "SV_APP_SWITCH": "app sw",
    "SV_SELECT_NONE": "desel",
    "SV_SELECT_WORD": "sel wd",
    "SV_EXTEND_WORD": "ext wd",
    "SV_SELECT_LINE": "sel ln",
    "SV_EXTEND_LINE": "ext ln",
    "SV_TRIPLE_GRAVE": "```",
    "SV_MBO_SFT": "⇧",
    "SV_MBO_GUI": "⌘",
    "SV_MBO_ALT": "⌥",
    "SV_MBO_CTL": "⌃",
    "SV_LOCK_NAV": "🔒NAV",
    "SV_LOCK_NUM": "🔒NUM",
    "SV_LOCK_SYM": "🔒SYM",
    "SV_LOCK_FUNC": "🔒FN",
    "SV_LOCK_SYS": "🔒SYS",
    "SV_LOCK_MBO": "🔒MBO",
    "SV_LOCK_CLEAR": "🔒clr",
    "SV_BOOST_2": "boost2",
    "SV_BOOST_3": "boost3",
    "QK_REPEAT_KEY": "rpt",
}


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    text = re.sub(r"//.*", "", text)
    return text


def split_top_level(text: str) -> list[str]:
    parts: list[str] = []
    current: list[str] = []
    depth = 0

    for char in text:
        if char == "(":
            depth += 1
        elif char == ")":
            depth -= 1
        elif char == "," and depth == 0:
            token = "".join(current).strip()
            if token:
                parts.append(token)
            current = []
            continue
        current.append(char)

    tail = "".join(current).strip()
    if tail:
        parts.append(tail)
    return parts


def parse_object_defines(paths: Iterable[pathlib.Path]) -> dict[str, str]:
    defines: dict[str, str] = {}
    pattern = re.compile(r"^\s*#define\s+([A-Za-z_][A-Za-z0-9_]*)\s+(.*)$")
    for path in paths:
        text = path.read_text()
        for line in text.splitlines():
            match = pattern.match(line)
            if not match:
                continue
            name, value = match.groups()
            if "(" in name:
                continue
            defines[name] = value.strip()
    return defines


class EvalVisitor(ast.NodeVisitor):
    def __init__(self, env: dict[str, int]) -> None:
        self.env = env

    def visit_Expression(self, node: ast.Expression) -> int:
        return self.visit(node.body)

    def visit_Name(self, node: ast.Name) -> int:
        if node.id not in self.env:
            raise KeyError(node.id)
        return self.env[node.id]

    def visit_Constant(self, node: ast.Constant) -> int:
        if not isinstance(node.value, int):
            raise ValueError(node.value)
        return node.value

    def visit_UnaryOp(self, node: ast.UnaryOp) -> int:
        operand = self.visit(node.operand)
        if isinstance(node.op, ast.USub):
            return -operand
        if isinstance(node.op, ast.UAdd):
            return operand
        raise ValueError(ast.dump(node))

    def visit_BinOp(self, node: ast.BinOp) -> int:
        left = self.visit(node.left)
        right = self.visit(node.right)
        if isinstance(node.op, ast.Add):
            return left + right
        if isinstance(node.op, ast.Sub):
            return left - right
        if isinstance(node.op, ast.Mult):
            return left * right
        if isinstance(node.op, ast.FloorDiv):
            return left // right
        raise ValueError(ast.dump(node))

    def generic_visit(self, node: ast.AST) -> int:
        raise ValueError(ast.dump(node))


def eval_int_expr(expr: str, env: dict[str, int]) -> int:
    tree = ast.parse(expr, mode="eval")
    return EvalVisitor(env).visit(tree)


def parse_layers_enum(text: str, defines: dict[str, str], dynamic_layer_count: int) -> dict[str, int]:
    cleaned = strip_comments(text)
    match = re.search(r"enum\s+layer\s*\{(.*?)\};", cleaned, flags=re.S)
    if not match:
        raise ValueError("Could not find enum layer")

    env: dict[str, int] = {"DYNAMIC_KEYMAP_LAYER_COUNT": dynamic_layer_count}
    if "MH_AUTO_BUTTONS_LAYER" in defines:
        env["MH_AUTO_BUTTONS_LAYER"] = eval_int_expr(defines["MH_AUTO_BUTTONS_LAYER"], env)

    layers: dict[str, int] = {}
    value = 0
    for entry in split_top_level(match.group(1)):
        if "=" in entry:
            name, expr = [part.strip() for part in entry.split("=", 1)]
            value = eval_int_expr(expr, {**env, **layers})
        else:
            name = entry.strip()
        layers[name] = value
        value += 1
    return layers


def extract_layout_blocks(text: str) -> dict[str, list[str]]:
    cleaned = strip_comments(text)
    blocks: dict[str, list[str]] = {}
    pattern = re.compile(r"\[(\w+)\]\s*=\s*LAYOUT\(")

    for match in pattern.finditer(cleaned):
        layer_name = match.group(1)
        start = match.end()
        depth = 1
        index = start
        while index < len(cleaned) and depth:
            char = cleaned[index]
            if char == "(":
                depth += 1
            elif char == ")":
                depth -= 1
            index += 1
        if depth != 0:
            raise ValueError(f"Unbalanced LAYOUT() for {layer_name}")
        body = cleaned[start:index - 1]
        blocks[layer_name] = split_top_level(body)
    return blocks


def load_slot_labels() -> list[str]:
    info = json.loads(INFO_PATH.read_text())
    return [item["label"] for item in info["layouts"]["LAYOUT"]["layout"]]


def load_custom_labels() -> dict[str, str]:
    custom = json.loads(VIAL_PATH.read_text()).get("customKeycodes", [])
    labels = {}
    for item in custom:
        short_name = re.sub(r"\s+", " ", item["shortName"].replace("\n", " ")).strip()
        labels[item["name"]] = short_name
    labels.update(CUSTOM_LABEL_OVERRIDES)
    return labels


def resolve_alias(token: str, defines: dict[str, str]) -> str:
    seen: set[str] = set()
    token = token.strip()
    while re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", token) and token in defines and token not in seen:
        seen.add(token)
        token = defines[token].strip()
    return token


def parse_call(token: str) -> tuple[str, list[str]] | None:
    token = token.strip()
    match = re.fullmatch(r"([A-Za-z_][A-Za-z0-9_]*)\((.*)\)", token)
    if not match:
        return None
    return match.group(1), split_top_level(match.group(2))


def normalize_name(token: str) -> str:
    token = token.replace("KC_", "")
    token = token.replace("_", " ")
    return token.lower()


def resolve_layer_name(raw: str, layers: dict[str, int]) -> str:
    """Return a human-readable layer label.  Prefer the symbolic name."""
    raw = raw.strip()
    if raw in layers:
        return raw
    if raw.isdigit():
        index = int(raw)
        index_to_name = {v: k for k, v in layers.items()}
        if index in index_to_name:
            return index_to_name[index]
        return raw
    return raw


def format_plain_key(token: str, custom_labels: dict[str, str]) -> str:
    token = token.strip()
    if token in custom_labels:
        return custom_labels[token]
    if token in KEY_LABELS:
        return KEY_LABELS[token]
    if re.fullmatch(r"KC_F\d{1,2}", token):
        return token.removeprefix("KC_")
    if token.startswith("RGB_"):
        return normalize_name(token)
    if token.startswith("SV_"):
        return custom_labels.get(token, normalize_name(token))
    return normalize_name(token)


def format_key(token: str, defines: dict[str, str], layers: dict[str, int], custom_labels: dict[str, str]) -> str:
    token = resolve_alias(token, defines)
    if token in ("KC_NO", "KC_TRNS"):
        return ""

    call = parse_call(token)
    if call is None:
        return format_plain_key(token, custom_labels)

    func, args = call

    if func == "LT" and len(args) == 2:
        tap = format_key(args[1], defines, layers, custom_labels)
        layer = resolve_layer_name(args[0], layers)
        return layer if not tap else f"{tap} ({layer})"

    if func == "MO" and len(args) == 1:
        return f"MO {resolve_layer_name(args[0], layers)}"

    if func == "TO" and len(args) == 1:
        return f"TO {resolve_layer_name(args[0], layers)}"

    if func == "TG" and len(args) == 1:
        return f"TG {resolve_layer_name(args[0], layers)}"

    if func in MOD_TAP_NAMES and len(args) == 1:
        tap = format_key(args[0], defines, layers, custom_labels)
        return f"{tap} ({MOD_TAP_NAMES[func]})" if tap else MOD_TAP_NAMES[func]

    if func == "MT" and len(args) == 2:
        tap = format_key(args[1], defines, layers, custom_labels)
        hold = normalize_name(args[0])
        return f"{tap} ({hold})" if tap else hold

    if func == "S" and len(args) == 1:
        base = resolve_alias(args[0], defines)
        if base in SHIFTED_KEY_LABELS:
            return SHIFTED_KEY_LABELS[base]
        return f"⇧{format_key(base, defines, layers, custom_labels)}"

    if func in MOD_PREFIXES and len(args) == 1:
        inner = format_key(args[0], defines, layers, custom_labels)
        mods = "".join(MOD_PREFIXES[func])
        return mods if not inner else f"{mods}{inner}"

    if func == "OSM" and len(args) == 1:
        mod = args[0].strip()
        return OSM_MOD_LABELS.get(mod, f"1s{normalize_name(mod)}")

    return format_plain_key(token, custom_labels)


def legend_text_colors(line_count: int) -> str:
    if line_count > len(LEGEND_COLORS):
        raise ValueError(
            f"Only {len(LEGEND_COLORS)} legend colors defined, cannot render {line_count} lines"
        )
    return "\n".join(LEGEND_COLORS[:line_count])


def legend_font_sizes(line_count: int) -> list[int]:
    if line_count > len(LEGEND_FONT_SIZES):
        raise ValueError(
            f"Only {len(LEGEND_FONT_SIZES)} legend font sizes defined, cannot render {line_count} lines"
        )
    return LEGEND_FONT_SIZES[:line_count]


def build_layer_slot_map(
    keymap_path: pathlib.Path,
    dynamic_layer_count: int,
) -> tuple[dict[str, dict[str, str]], dict[str, int], dict[str, str], list[str]]:
    keymap_text = keymap_path.read_text()
    defines = parse_object_defines([KEYMAP_SUPPORT_PATH, keymap_path])
    layers = parse_layers_enum(keymap_text, defines, dynamic_layer_count)
    blocks = extract_layout_blocks(keymap_text)
    slot_labels = load_slot_labels()

    layer_slot_map: dict[str, dict[str, str]] = {}
    for layer_name, tokens in blocks.items():
        if len(tokens) != len(slot_labels):
            raise ValueError(
                f"Layer {layer_name} has {len(tokens)} tokens, expected {len(slot_labels)}"
            )
        layer_slot_map[layer_name] = dict(zip(slot_labels, tokens))
    return layer_slot_map, layers, defines, slot_labels


def render_label(
    slot_name: str,
    selected_layers: list[str],
    layer_slot_map: dict[str, dict[str, str]],
    defines: dict[str, str],
    layers: dict[str, int],
    custom_labels: dict[str, str],
    line_count: int,
) -> str:
    lines: list[str] = []
    for layer_name in selected_layers:
        token = layer_slot_map[layer_name][slot_name]
        resolved = resolve_alias(token, defines)
        call = parse_call(resolved)
        if (
            layer_name == "BASE"
            and slot_name in BASE_HOME_ROW_MOD_SLOTS
            and call is not None
            and call[0] in MOD_TAP_NAMES
            and len(call[1]) == 1
        ):
            lines.append(format_key(call[1][0], defines, layers, custom_labels))
            continue
        lines.append(format_key(token, defines, layers, custom_labels))
    while len(lines) < line_count:
        lines.append("")
    return "\n".join(lines)


def update_kle_template(
    template: list[object],
    selected_layers: list[str],
    layer_slot_map: dict[str, dict[str, str]],
    defines: dict[str, str],
    layers: dict[str, int],
    custom_labels: dict[str, str],
    line_count: int,
) -> list[object]:
    slot_iter = iter(KLE_PHYSICAL_ORDER)
    updated: list[object] = [template[0]]
    text_colors = legend_text_colors(line_count)
    font_sizes = legend_font_sizes(line_count)

    for row in template[1:]:
        if not isinstance(row, list):
            updated.append(row)
            continue

        new_row: list[object] = []
        pending_props: dict[str, object] = {}
        for item in row:
            if not isinstance(item, str):
                pending_props.update(item)
                continue
            if item == "Trackpoint":
                if pending_props:
                    new_row.append(pending_props)
                    pending_props = {}
                new_row.append(item)
                continue
            slot_name = next(slot_iter, None)
            if slot_name is None:
                raise ValueError("KLE template has more physical keys than expected")
            props = dict(pending_props)
            props["t"] = text_colors
            props["fa"] = font_sizes
            props["a"] = LEGEND_ALIGNMENT
            if props:
                new_row.append(props)
            pending_props = {}
            new_row.append(
                render_label(
                    slot_name,
                    selected_layers,
                    layer_slot_map,
                    defines,
                    layers,
                    custom_labels,
                    line_count,
                )
            )
        if pending_props:
            new_row.append(pending_props)
        updated.append(new_row)

    if next(slot_iter, None) is not None:
        raise ValueError("KLE template has fewer physical keys than expected")
    return updated


def parse_selected_layers(raw: str, layer_slot_map: dict[str, dict[str, str]], layers: dict[str, int]) -> list[str]:
    ordered = {index: name for name, index in layers.items()}
    selected: list[str] = []
    for part in [piece.strip() for piece in raw.split(",") if piece.strip()]:
        if part.isdigit():
            layer_name = ordered.get(int(part))
            if layer_name is None:
                raise ValueError(f"Unknown layer index {part}")
            part = layer_name
        if part not in layer_slot_map:
            raise ValueError(f"Unknown layer {part}")
        selected.append(part)
    return selected


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Rewrite a Svalboard KLE JSON template with legends from a QMK keymap."
    )
    parser.add_argument(
        "--template",
        type=pathlib.Path,
        required=True,
        help="Input KLE JSON template to rewrite.",
    )
    parser.add_argument(
        "--output",
        type=pathlib.Path,
        required=True,
        help="Output KLE JSON path.",
    )
    parser.add_argument(
        "--keymap",
        type=pathlib.Path,
        default=KEYMAP_PATH,
        help=f"Keymap source to parse. Default: {KEYMAP_PATH}",
    )
    parser.add_argument(
        "--layers",
        default="BASE,NAV,NUM,SYM",
        help="Comma-separated layer names or indices to render into the KLE legends.",
    )
    parser.add_argument(
        "--legend-lines",
        type=int,
        default=4,
        help="Number of newline-separated legend slots to emit per key. Default: 4.",
    )
    parser.add_argument(
        "--dynamic-layer-count",
        type=int,
        default=16,
        help="Value to use for DYNAMIC_KEYMAP_LAYER_COUNT when resolving layer expressions.",
    )
    args = parser.parse_args()

    layer_slot_map, layers, defines, _slot_labels = build_layer_slot_map(
        args.keymap,
        args.dynamic_layer_count,
    )
    selected_layers = parse_selected_layers(args.layers, layer_slot_map, layers)
    if len(selected_layers) > args.legend_lines:
        raise ValueError(
            f"{len(selected_layers)} layers selected but only {args.legend_lines} legend lines available"
        )

    custom_labels = load_custom_labels()
    template = json.loads(args.template.read_text())
    updated = update_kle_template(
        template,
        selected_layers,
        layer_slot_map,
        defines,
        layers,
        custom_labels,
        args.legend_lines,
    )

    args.output.write_text(json.dumps(updated, indent=2) + "\n")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        raise
