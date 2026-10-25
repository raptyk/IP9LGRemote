# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#
# Checks that UI strings fit the 240 px Cardputer screen, using the real glyph widths
# of the M5GFX fonts. Run after `pio run` (needs the downloaded M5GFX library):
#   python tools/check_text_fit.py
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
FONTS = ROOT / ".pio/libdeps/cardputer/M5GFX/src/lgfx/Fonts"


def load_gfx_font(name):
    text = (FONTS / "GFXFF" / f"{name}.h").read_text(encoding="utf-8", errors="ignore")
    glyphs = re.findall(r"\{\s*\d+,\s*\d+,\s*\d+,\s*(\d+),\s*-?\d+,\s*-?\d+\s*\}", text)
    first = int(re.search(r"0x([0-9A-Fa-f]+),\s*0x[0-9A-Fa-f]+,\s*\d+\s*\};", text).group(1), 16)
    return {chr(first + i): int(advance) for i, advance in enumerate(glyphs)}


def load_font2():
    text = (FONTS / "Font16.h").read_text(encoding="utf-8", errors="ignore")
    table = re.search(r"widtbl_f16\[96\]\s*=.*?\{(.*?)\};", text, re.S).group(1)
    widths = [int(v, 0) for v in re.findall(r"0x[0-9A-Fa-f]+|\d+", re.sub(r"//.*", "", table))]
    return {chr(32 + i): w for i, w in enumerate(widths)}


def width(font, text):
    if font == "Font0":
        return 6 * len(text)
    return sum(FONT_TABLES[font].get(c, 8) for c in text)


def wrap(font, text, max_width):
    lines, line = [], ""
    for word in text.split(" "):
        candidate = f"{line} {word}" if line else word
        if width(font, candidate) <= max_width or not line:
            line = candidate
        else:
            lines.append(line)
            line = word
    if line:
        lines.append(line)
    return lines


FONT_TABLES = {
    "FreeSans9pt7b": load_gfx_font("FreeSans9pt7b"),
    "FreeSansBold9pt7b": load_gfx_font("FreeSansBold9pt7b"),
    "FreeSansBold18pt7b": load_gfx_font("FreeSansBold18pt7b"),
    "Font2": load_font2(),
}

STRING_PATTERN = r'constexpr const char\* (k\w+) = "([^"]*)";'
strings = dict(re.findall(STRING_PATTERN, (ROOT / "src/ui/Strings.h").read_text(encoding="utf-8")))
# Footers of the shared screens live in IP9Kit: the checkout next to the app, or the downloaded copy.
for kit in (ROOT.parent / "IP9Kit", ROOT / ".pio/libdeps/cardputer/IP9Kit"):
    kit_strings = kit / "src/ip9/ui/Strings.h"
    if kit_strings.is_file():
        strings.update(re.findall(STRING_PATTERN, kit_strings.read_text(encoding="utf-8")))
        break

# (name pattern, font, max width, max lines)
RULES = [
    (r"^kFooter", "Font0", 236, 1),
    (r"Text$", "Font0", 228, 2),  # list descriptions / card texts
    (r"^kCard\w+Title$|^kStep(Wifi|TvPrep|FindTv|Pair|IrTest|BleCheck|BlePair|UsbCable|Start)$|^kMenu(?!.*Text)\w+$|^kTv(Search|Manual|Forget|PairAgain|Keycode|Mac|PairingMethod)$",
     "FreeSans9pt7b", 200, 1),
    (r"^kPair\w+Title$|^kIrTestQuestion$", "FreeSansBold9pt7b", 228, 1),
    (r"Hint$|^kPair\w+Text$|^kPairChecks$", "Font2", 228, 3),
    (r"^kHomeStatus$|^kUsbNotConnected$|^kInfrared$", "Font0", 130, 1),
    (r"^kStatus\w+Detail$", "Font2", 188, 2),         # next to the connection badge
    (r"^kStatus(?!.*Detail)", "FreeSansBold9pt7b", 188, 1),
    (r"^kBand", "FreeSansBold9pt7b", 200, 1),          # coloured mode band
    (r"^kPointerHint", "Font2", 160, 3),               # next to the crosshair
]

problems = 0
for name, value in sorted(strings.items()):
    for pattern, font, max_width, max_lines in RULES:
        if re.search(pattern, name):
            lines = wrap(font, value, max_width) if max_lines > 1 else [value]
            too_wide = max_lines == 1 and width(font, value) > max_width
            if too_wide or len(lines) > max_lines:
                problems += 1
                print(f"{name}: {len(lines)} line(s), {width(font, value)} px in {font} -> {value!r}")
            break

# Feedback labels switch to the 12 pt font above 200 px (see RemoteScreen::drawFeedback).
FONT_TABLES["FreeSansBold12pt7b"] = load_gfx_font("FreeSansBold12pt7b")
labels = ["QUICK MENU", "PLAY/PAUSE", "POWER OFF", "SETTINGS", "FORWARD", "EZ ADJUST", "IN START"]
for label in labels:
    font = "FreeSansBold18pt7b" if width("FreeSansBold18pt7b", label) <= 200 else "FreeSansBold12pt7b"
    if width(font, label) > 216:
        problems += 1
        print(f"feedback label too wide: {label} {width('FreeSansBold18pt7b', label)} px")

# Key-cap row of the remote screen: caps (min 16 px, text + 8) + 4 px + label, three per row.
def key_hint(key, label):
    return max(16, width("Font0", key) + 8) + 4 + width("Font0", label)


for row in (
    [("kKeyTab", "kKeyHelp"), ("kKeyOpt", "kKeyMenu"), ("kKeyA", "kKeyAirMouse")],
    [("kKeyEnter", "kKeyClick"), ("kKeyBrackets", "kKeyScroll"), ("kKeyA", "kKeyExit")],
):
    total = sum(key_hint(strings[k], strings[l]) for k, l in row)
    if total + 4 * 8 > 240:
        problems += 1
        print(f"key row too wide: {total} px -> {row}")

print("OK" if problems == 0 else f"{problems} problem(s)")
sys.exit(1 if problems else 0)
