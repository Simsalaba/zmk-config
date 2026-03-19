#!/usr/bin/env python3

import json
from pathlib import Path

import yaml


ROOT = Path(__file__).resolve().parents[1]
KEYMAP_YAML = ROOT / "keymap-drawer" / "corne.yaml"
OUTPUT_JSON = ROOT / "keymap-drawer" / "corne.kle.json"

LEFT_COLORS = ["#000000", "#ff0000", "#00aa00", "#2040d0", "#ffffff"]
RIGHT_COLORS = ["#000000", "#ff00ff", "#c59a00", "#00a8c0", "#ffffff"]
SPECIAL_LABELS = {"BOOT", "TAP", "EXTRA", "BASE", "NUM", "NAV", "SYM", "MOUSE", "MEDIA", "FUN"}
KEYCAP_COLOR = "#d7d7d7"
BLANK_COLOR = "#a6a6a6"
HOLD_COLOR = "#555555"
SPECIAL_COLOR_MAP = {
    "FUN": "#ff0000",
    "SYM": "#00aa00",
    "NUM": "#2040d0",
    "MEDIA": "#ff00ff",
    "NAV": "#c59a00",
    "MOUSE": "#00a8c0",
}

POSITIONS = [
    {"x": 0.00, "y": 0.37}, {"x": 1.00, "y": 0.37}, {"x": 2.00, "y": 0.12}, {"x": 3.00, "y": 0.00},
    {"x": 4.00, "y": 0.12}, {"x": 5.00, "y": 0.24}, {"x": 8.00, "y": 0.24}, {"x": 9.00, "y": 0.12},
    {"x": 10.00, "y": 0.00}, {"x": 11.00, "y": 0.12}, {"x": 12.00, "y": 0.37}, {"x": 13.00, "y": 0.37},
    {"x": 0.00, "y": 1.37}, {"x": 1.00, "y": 1.37}, {"x": 2.00, "y": 1.12}, {"x": 3.00, "y": 1.00},
    {"x": 4.00, "y": 1.12}, {"x": 5.00, "y": 1.24}, {"x": 8.00, "y": 1.24}, {"x": 9.00, "y": 1.12},
    {"x": 10.00, "y": 1.00}, {"x": 11.00, "y": 1.12}, {"x": 12.00, "y": 1.37}, {"x": 13.00, "y": 1.37},
    {"x": 0.00, "y": 2.37}, {"x": 1.00, "y": 2.37}, {"x": 2.00, "y": 2.12}, {"x": 3.00, "y": 2.00},
    {"x": 4.00, "y": 2.12}, {"x": 5.00, "y": 2.24}, {"x": 8.00, "y": 2.24}, {"x": 9.00, "y": 2.12},
    {"x": 10.00, "y": 2.00}, {"x": 11.00, "y": 2.12}, {"x": 12.00, "y": 2.37}, {"x": 13.00, "y": 2.37},
    {"x": 3.50, "y": 3.12},
    {"x": 4.50, "y": 3.12, "r": 12, "rx": 4.50, "ry": 4.12},
    {"x": 5.48, "y": 2.83, "h": 1.5, "r": 24, "rx": 5.48, "ry": 4.33},
    {"x": 7.52, "y": 2.83, "h": 1.5, "r": -24, "rx": 8.52, "ry": 4.33},
    {"x": 8.50, "y": 3.12, "r": -12, "rx": 9.50, "ry": 4.12},
    {"x": 9.50, "y": 3.12},
]

# KLE handles the Corne thumb rotations poorly. Use a cleaner editable
# approximation here while keeping the real matrix stagger for the alphas.
KLE_POSITIONS = POSITIONS[:36] + [
    {"x": 3.50, "y": 3.30},
    {"x": 4.55, "y": 3.48},
    {"x": 5.72, "y": 3.18, "h": 1.5},
    {"x": 7.28, "y": 3.18, "h": 1.5},
    {"x": 8.45, "y": 3.48},
    {"x": 9.50, "y": 3.30},
]


DISPLAY_MAP = {
    "&bootloader": "BOOT",
    "&u_to_U_TAP": "TAP",
    "&u_to_U_EXTRA": "EXTRA",
    "&u_to_U_BASE": "BASE",
    "&u_to_U_NUM": "NUM",
    "&u_to_U_NAV": "NAV",
    "&u_to_U_SYM": "SYM",
    "&u_to_U_MOUSE": "MOUSE",
    "&u_to_U_MEDIA": "MEDIA",
    "&u_to_U_FUN": "FUN",
    "&caps_word": "CAP",
    "&mmv MOVE_LEFT": "M←",
    "&mmv MOVE_DOWN": "M↓",
    "&mmv MOVE_UP": "M↑",
    "&mmv MOVE_RIGHT": "M→",
    "&msc SCRL_LEFT": "S←",
    "&msc SCRL_DOWN": "S↓",
    "&msc SCRL_UP": "S↑",
    "&msc SCRL_RIGHT": "S→",
    "&mkp MB1": "L",
    "&mkp MB2": "M",
    "&mkp MB3": "R",
    "Sft+INS": "PST",
    "Ctl+INS": "CPY",
    "Sft+DEL": "CUT",
    "AGAIN": "RDO",
    "PG DN": "PGDN",
    "PG UP": "PGUP",
    "PAUSE BREAK": "PAUSE",
    "PSCRN": "PRTSC",
    "SLCK": "SLCK",
    "SPACE": "Spc",
    "RET": "↵",
    "BSPC": "⌫",
    "DEL": "⌦",
    "ESC": "Esc",
    "TAB": "⭾",
    "APP": "≣",
    "LGUI": "GUI",
    "RGUI": "GUI",
    "LALT": "ALT",
    "RALT": "ALT",
    "LCTRL": "CTL",
    "RCTRL": "CTL",
    "LSHFT": "SFT",
    "RSHFT": "SFT",
    "Button": "BTN",
}

ICON_MAP = {
    "PREV": "⏮",
    "NEXT": "⏭",
    "VOL DN": "🔉",
    "VOL UP": "🔊",
    "MUTE": "🔇",
    "PP": "⏯",
    "STOP": "◼",
    "LEFT": "⬅",
    "DOWN": "⬇",
    "UP": "⬆",
    "RIGHT": "➡",
    "HOME": "⇱",
    "END": "⇲",
    "PGDN": "⇟",
    "PGUP": "⇞",
    "INS": "INS",
    "M←": "◀",
    "M↓": "▼",
    "M↑": "▲",
    "M→": "▶",
    "S←": "⇦",
    "S↓": "⇩",
    "S↑": "⇧",
    "S→": "⇨",
    "BT0": "ᛒ0",
    "BT1": "ᛒ1",
    "BT2": "ᛒ2",
    "BT3": "ᛒ3",
    "OUT TOG": "OUT",
    "RGB TOG": "RGB",
    "RGB EFF": "FX",
    "RGB HUI": "H+",
    "RGB SAI": "S+",
    "RGB BRI": "B+",
    "EP TOG": "EP",
    "UNDO": "UND",
}


def label(value):
    if isinstance(value, dict):
        tap = str(value.get("t", ""))
        if tap == "BT" and "h" in value:
            return f"BT{value['h']}"
        return DISPLAY_MAP.get(tap, tap)
    if value is None:
        return ""
    text = str(value)
    return DISPLAY_MAP.get(text, text)


def entry_tap(value):
    return label(value)


def entry_hold(value):
    if isinstance(value, dict) and "h" in value:
        return DISPLAY_MAP.get(str(value["h"]), str(value["h"]))
    return ""


def condensed_legends(layers, row_idx, col_idx):
    base_entry = layers["Base"][row_idx][col_idx]
    base = entry_tap(base_entry)
    hold = entry_hold(base_entry)
    show_hold = row_idx in {1, 3}
    if not show_hold:
        hold = ""

    is_thumb = row_idx == 3
    is_left = col_idx < 6 if not is_thumb else col_idx < 3

    if is_left:
        legends = [
            base,
            entry_tap(layers["Fun"][row_idx][col_idx]),
            entry_tap(layers["Sym"][row_idx][col_idx]),
            entry_tap(layers["Num"][row_idx][col_idx]),
            hold,
        ]
        colors = LEFT_COLORS
        colors = colors[:]
        special_key = legends[4].upper() if legends[4] else ""
        if special_key in SPECIAL_COLOR_MAP:
            colors[4] = SPECIAL_COLOR_MAP[special_key]
        elif legends[4]:
            colors[4] = HOLD_COLOR
    else:
        legends = [
            base,
            entry_tap(layers["Media"][row_idx][col_idx]),
            entry_tap(layers["Nav"][row_idx][col_idx]),
            entry_tap(layers["Mouse"][row_idx][col_idx]),
            hold,
        ]
        colors = RIGHT_COLORS

        # Mirror the upstream reference more closely by using compact icons
        # on the right side and suppressing repeated helper legends.
        legends[1:] = [ICON_MAP.get(item, item) for item in legends[1:]]
        seen = {legends[0]}
        for idx in range(1, len(legends)):
            item = legends[idx]
            if not item:
                continue
            if item in seen:
                legends[idx] = ""
            else:
                seen.add(item)
        colors = colors[:]
        special_key = legends[4].upper() if legends[4] else ""
        if special_key in SPECIAL_COLOR_MAP:
            colors[4] = SPECIAL_COLOR_MAP[special_key]
        elif legends[4]:
            colors[4] = HOLD_COLOR

    is_blank = not any(legends)
    return {"legends": legends, "colors": colors, "is_blank": is_blank}


def build_keys(layers):
    out = []
    for row_idx, row in enumerate(layers["Base"]):
        for col_idx, _ in enumerate(row):
            out.append(condensed_legends(layers, row_idx, col_idx))
    return out


def row_from_indices(keys, indices, prev_row_y, default=None):
    row = []
    current_x = 0.0
    current_y = prev_row_y + 1.0
    row_start_y = None

    for idx_in_row, key_idx in enumerate(indices):
        pos = KLE_POSITIONS[key_idx]
        key = keys[key_idx]
        entry = {}

        if idx_in_row == 0 and default is not None:
            entry.update(default)

        dx = pos["x"] - current_x
        dy = pos["y"] - current_y
        if dx:
            entry["x"] = dx
        if dy:
            entry["y"] = dy

        if "r" in pos:
            entry["r"] = pos["r"]
            entry["rx"] = pos["rx"]
            entry["ry"] = pos["ry"]
        if "h" in pos:
            entry["h"] = pos["h"]

        if key["is_blank"]:
            entry["c"] = BLANK_COLOR
            entry["t"] = BLANK_COLOR
        else:
            entry["c"] = KEYCAP_COLOR
            entry["t"] = "\n".join(key["colors"])
            entry["f"] = 3
            entry["a"] = 4
            entry["fa"] = [0, 0, 2, 2, 2]

        row.append(entry)
        row.append("\n".join(key["legends"]))

        current_x = pos["x"] + 1.0
        current_y = pos["y"]
        if row_start_y is None:
            row_start_y = pos["y"]

    return row, row_start_y if row_start_y is not None else prev_row_y


def main():
    data = yaml.safe_load(KEYMAP_YAML.read_text())
    layers = data["layers"]
    keys = build_keys(layers)

    json_data = [
        {
            "name": "Corne Miryoku QWERTY Swedish Reference",
            "author": "Codex",
            "notes": "Generated from keymap-drawer/corne.yaml",
        },
        [
            {
                "d": True,
                "w": 14,
                "h": 0.5,
                "f": 4,
            },
            "<b>Miryoku ZMK</b>",
        ],
    ]

    prev_y = -1.0
    for indices in [
        list(range(0, 12)),
        list(range(12, 24)),
        list(range(24, 36)),
        list(range(36, 42)),
    ]:
        row, prev_y = row_from_indices(keys, indices, prev_y)
        json_data.append(row)

    OUTPUT_JSON.write_text(json.dumps(json_data, ensure_ascii=False, indent=2) + "\n")


if __name__ == "__main__":
    main()
