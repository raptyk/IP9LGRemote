# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#
# Renders mock-ups of the firmware screens for the README (docs/images/*.png).
# Layout, sizes, colours and texts follow src/ui (Theme.h, Strings.h, the screen classes);
# fonts are approximated with Arial. Usage: python tools/make_screenshots.py
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "docs" / "images"
FONT_DIR = Path("C:/Windows/Fonts")
S = 2  # pixel scale of the 240x135 screen

BG = (10, 10, 14)
SURFACE = (28, 28, 36)
SURFACE_HIGH = (48, 48, 60)
TEXT = (236, 236, 240)
MUTED = (140, 140, 155)
DISABLED = (80, 80, 92)
ACCENT = (230, 0, 110)
OK = (60, 200, 110)
WARN = (245, 170, 40)
MODE = {"wifi": (30, 120, 230), "ir": (225, 80, 30), "ble": (110, 80, 220), "usb": (30, 160, 90)}
TINT = {"wifi": (8, 22, 46), "ir": (42, 16, 6), "ble": (24, 16, 48), "usb": (6, 32, 18)}
CHIP = {"wifi": "WI-FI", "ir": "IR", "ble": "BT", "usb": "USB"}


def font(px, bold=False):
    return ImageFont.truetype(str(FONT_DIR / ("arialbd.ttf" if bold else "arial.ttf")), int(px * S))


F0 = lambda: font(7.5)          # Font0 (6x8)
F0B = lambda: font(7.5, True)
F2 = lambda: font(10.5)         # Font2 (16 px)
F9 = lambda: font(11)           # FreeSans9pt
F9B = lambda: font(11, True)    # FreeSansBold9pt
F18B = lambda: font(21, True)   # FreeSansBold18pt


class Screen:
    def __init__(self):
        self.image = Image.new("RGB", (240 * S, 135 * S), BG)
        self.d = ImageDraw.Draw(self.image)

    def r(self, x0, y0, x1, y1, fill, radius=0, outline=None):
        box = [x0 * S, y0 * S, x1 * S, y1 * S]
        if radius:
            self.d.rounded_rectangle(box, radius=radius * S, fill=fill, outline=outline, width=S)
        else:
            self.d.rectangle(box, fill=fill, outline=outline)

    def t(self, x, y, text, f, fill=TEXT, anchor="la"):
        self.d.text((x * S, y * S), text, font=f, fill=fill, anchor=anchor)

    def circle(self, cx, cy, radius, fill):
        self.d.ellipse([(cx - radius) * S, (cy - radius) * S, (cx + radius) * S, (cy + radius) * S], fill=fill)

    def line(self, points, fill, width=1.0):
        self.d.line([(x * S, y * S) for x, y in points], fill=fill, width=max(1, int(width * S)))

    # --- common parts -------------------------------------------------------------
    def status_bar(self, chip, chip_colour, text, dot):
        self.r(0, 0, 240, 18, SURFACE)
        f = F0B()
        width = self.d.textlength(chip, font=f) / S + 12
        self.r(2, 2, 2 + width, 16, chip_colour, radius=4)
        self.t(2 + width / 2, 9, chip, f, anchor="mm")
        self.circle(2 + width + 8, 9, 3, dot)
        self.t(2 + width + 15, 9, text, F0(), anchor="lm")
        self.r(216, 5, 234, 13, None, outline=TEXT)
        self.r(218, 7, 230, 11, TEXT)
        self.t(213, 9, "87%", F0(), MUTED, anchor="rm")

    def footer(self, text):
        self.r(0, 119, 240, 135, SURFACE)
        self.t(120, 127, text, F0(), MUTED, anchor="mm")

    def title(self, text, top=18):
        self.t(6, top + 3, text, F9B())
        self.r(6, top + 18, 234, top + 18.6, SURFACE_HIGH)
        return top + 20

    def key_hints(self, top, hints):
        f, lf = F0B(), F0()
        widths = []
        for key, label in hints:
            cap = max(16, self.d.textlength(key, font=f) / S + 8)
            widths.append((cap, cap + 4 + self.d.textlength(label, font=lf) / S))
        gap = (240 - sum(w for _, w in widths)) / (len(hints) + 1)
        x = gap
        cy = top + 12.5
        for (key, label), (cap, total) in zip(hints, widths):
            self.r(x, cy - 6.5, x + cap, cy + 7.5, SURFACE, radius=3)
            self.r(x, cy - 7.5, x + cap, cy + 5.5, TEXT, radius=3)
            self.t(x + cap / 2, cy - 1, key, f, BG, anchor="mm")
            self.t(x + cap + 4, cy, label, lf, anchor="lm")
            x += total + gap

    def wifi_icon(self, cx, cy, k, colour):
        base = cy + 14 * k
        for i in range(3):
            radius = (10 + i * 8) * k
            self.d.arc([(cx - radius) * S, (base - radius) * S, (cx + radius) * S, (base + radius) * S], 225, 315,
                       fill=colour, width=max(2, int(4 * k * S)))
        self.circle(cx, base - 2 * k, max(1.5, 3 * k), colour)

    def ir_icon(self, cx, cy, k, colour):
        w, h = 14 * k, 24 * k
        x, y = cx - w / 2, cy - 4 * k
        self.r(x, y, x + w, y + h, None, radius=3 * k, outline=colour)
        self.circle(cx, y + 6 * k, 2 * k, ACCENT)
        for i in range(2):
            radius = (9 + i * 6) * k
            self.d.arc([(cx - radius) * S, (y - 2 * k - radius) * S, (cx + radius) * S, (y - 2 * k + radius) * S],
                       240, 300, fill=colour, width=max(2, int(2 * k * S)))

    def band(self, mode, title, via=None):
        self.r(0, 18, 240, 135, TINT[mode])
        self.r(0, 18, 240, 42, MODE[mode])
        (self.wifi_icon if mode == "wifi" else self.ir_icon)(16, 30, 0.45, TEXT)
        self.t(32, 31, title, F9B(), anchor="lm")
        if via:
            self.t(234, 30, via, F0(), anchor="rm")

    def save(self, name):
        OUT.mkdir(parents=True, exist_ok=True)
        self.image.save(OUT / name, optimize=True)


# --- screens --------------------------------------------------------------------------
def home():
    s = Screen()
    s.status_bar("LG", ACCENT, "Choose a mode", ACCENT)
    # neighbour previews and chevrons
    s.ir_icon(34, 42, 0.55, SURFACE_HIGH)
    s.d.polygon([(6 * S, 42 * S), (12 * S, 36 * S), (12 * S, 48 * S)], fill=MUTED)
    s.d.polygon([(234 * S, 42 * S), (228 * S, 36 * S), (228 * S, 48 * S)], fill=MUTED)
    s.r(94, 20, 146, 64, MODE["wifi"], radius=10)
    s.wifi_icon(120, 42, 1.0, TEXT)
    s.t(120, 67, "Wi-Fi remote", F9B(), anchor="ma")
    s.circle(52, 87, 3, OK)
    s.t(59, 87, "Ready: Living room TV", F0(), OK, anchor="lm")
    s.t(120, 96, "Pointer, typing and apps over your home", F0(), MUTED, anchor="ma")
    s.t(120, 106, "network. Falls back to IR.", F0(), MUTED, anchor="ma")
    s.footer("<> pick  enter open  s setup  tab help")
    s.save("home.png")


def remote_wifi():
    s = Screen()
    s.status_bar("WI-FI", MODE["wifi"], "Living room TV", OK)
    s.band("wifi", "WI-FI REMOTE")
    s.circle(24, 68, 13, OK)
    s.line([(18, 68), (22, 73), (30, 63)], TEXT, 2.5)
    s.t(46, 52, "Connected", F9B())
    s.t(46, 70, "Living room TV", F2(), MUTED)
    s.key_hints(94, [("TAB", "Help"), ("OPT", "Menu"), ("A", "Air mouse")])
    s.footer("t type  l apps  p power  0-9 digits")
    s.save("remote-wifi.png")


def remote_feedback():
    s = Screen()
    s.status_bar("WI-FI", MODE["wifi"], "Living room TV", OK)
    s.band("wifi", "WI-FI REMOTE", via="via WI-FI")
    f = F18B()
    width = s.d.textlength("VOL +", font=f) / S + 28
    s.r(120 - width / 2, 45, 120 + width / 2, 79, None, radius=10, outline=MUTED)
    s.t(120, 63, "VOL +", f, anchor="mm")
    s.r(31, 84, 181, 89, SURFACE_HIGH, radius=2)
    s.r(31, 84, 31 + 150 * 0.24, 89, ACCENT, radius=2)
    s.t(189, 87, "24", F0(), anchor="lm")
    s.key_hints(94, [("TAB", "Help"), ("OPT", "Menu"), ("A", "Air mouse")])
    s.footer("t type  l apps  p power  0-9 digits")
    s.save("remote-feedback.png")


def remote_ir():
    s = Screen()
    s.status_bar("IR", MODE["ir"], "Infrared", OK)
    s.band("ir", "IR REMOTE")
    s.circle(24, 68, 13, MODE["ir"])
    s.circle(24, 62, 2, TEXT)
    s.r(22, 66, 26, 76, TEXT, radius=2)
    s.t(46, 52, "Infrared", F9B())
    s.t(46, 70, "Aim the top edge (IR LED)", F2(), MUTED)
    s.t(46, 84, "at the TV.", F2(), MUTED)
    s.key_hints(94, [("TAB", "Help"), ("OPT", "Menu")])
    s.footer("p power  = - volume  ] [ channel")
    s.save("remote-ir.png")


def pointer():
    s = Screen()
    s.status_bar("WI-FI", MODE["wifi"], "Living room TV", OK)
    s.band("wifi", "AIR MOUSE")
    cx, cy = 44, 68
    s.d.ellipse([(cx - 12) * S, (cy - 12) * S, (cx + 12) * S, (cy + 12) * S], outline=ACCENT, width=2 * S)
    s.circle(cx, cy, 3, ACCENT)
    for x0, y0, x1, y1 in [(cx - 19, cy, cx - 11, cy), (cx + 12, cy, cx + 20, cy), (cx, cy - 19, cx, cy - 11),
                           (cx, cy + 12, cx, cy + 20)]:
        s.line([(x0, y0), (x1, y1)], ACCENT, 1)
    s.t(74, 61, "Move the remote to aim", F2(), anchor="lm")
    s.key_hints(94, [("ENTER", "Click"), ("[ ]", "Scroll"), ("A", "Exit")])
    s.footer("del back  esc exit pointer")
    s.save("pointer.png")


def setup():
    s = Screen()
    s.status_bar("WI-FI", MODE["wifi"], "Connecting (IR)", WARN)
    top = s.title("Setup: Wi-Fi remote")
    rows = [("1. Connect to Wi-Fi", "MyHome", "done"), ("2. Prepare the TV", "OK", "done"),
            ("3. Find the TV", "", "selected"), ("4. Pair", "", "locked")]
    for i, (label, value, state) in enumerate(rows[:3]):
        y = top + i * 18
        if state == "selected":
            s.r(3, y, 233, y + 17, ACCENT, radius=4)
        s.t(8, y + 9, label, F9(), TEXT, anchor="lm")
        if value:
            s.t(228, y + 9, value, F0(), TEXT if state == "selected" else MUTED, anchor="rm")
    s.r(6, 97, 234, 97.6, SURFACE_HIGH)
    s.t(6, 100, "Search the network, or type the IP (TV:", F0(), MUTED)
    s.t(6, 110, "Settings > Network > Advanced).", F0(), MUTED)
    s.footer("enter select  del back")
    s.save("setup.png")


def settings():
    s = Screen()
    s.status_bar("WI-FI", MODE["wifi"], "Living room TV", OK)
    top = s.title("Settings")
    rows = [("About", ""), ("Wi-Fi", "MyHome"), ("TV (network)", "Living room TV")]
    for i, (label, value) in enumerate(rows):
        y = top + i * 18
        if i == 1:
            s.r(3, y, 233, y + 17, ACCENT, radius=4)
        s.t(8, y + 9, label, F9(), anchor="lm")
        if value:
            s.t(228, y + 9, value, F0(), TEXT if i == 1 else MUTED, anchor="rm")
    s.r(6, 97, 234, 97.6, SURFACE_HIGH)
    s.t(6, 100, "Network the Cardputer joins. Must be the", F0(), MUTED)
    s.t(6, 110, "same as the TV's.", F0(), MUTED)
    s.footer("enter select  del back")
    s.save("settings.png")


def gallery():
    names = ["home", "remote-wifi", "remote-feedback", "pointer", "setup", "remote-ir"]
    images = [Image.open(OUT / f"{n}.png") for n in names]
    w, h = images[0].size
    pad = 16
    sheet = Image.new("RGB", (3 * w + 4 * pad, 2 * h + 3 * pad), (18, 18, 24))
    for i, img in enumerate(images):
        sheet.paste(img, (pad + (i % 3) * (w + pad), pad + (i // 3) * (h + pad)))
    sheet.save(OUT / "gallery.png", optimize=True)


if __name__ == "__main__":
    home()
    remote_wifi()
    remote_feedback()
    remote_ir()
    pointer()
    setup()
    settings()
    gallery()
    print(f"Written screens to {OUT}")
