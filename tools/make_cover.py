# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#
# Generates the M5Burner / store cover images:
#   publish/cover.png         1280x720
#   publish/cover-square.png  800x800
# Usage: python tools/make_cover.py   (needs Pillow; uses Arial from C:/Windows/Fonts)
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "publish"
FONT_DIR = Path("C:/Windows/Fonts")

# Same palette as src/ui/Theme.h.
BACKGROUND = (10, 10, 14)
SURFACE = (28, 28, 36)
TEXT = (236, 236, 240)
MUTED = (140, 140, 155)
ACCENT = (230, 0, 110)
OK = (60, 200, 110)
MODES = [("Wi-Fi", (30, 120, 230)), ("IR", (225, 80, 30)), ("Bluetooth", (110, 80, 220)), ("USB", (30, 160, 90))]
TINT_WIFI = (8, 22, 46)


def font(size, bold=False):
    return ImageFont.truetype(str(FONT_DIR / ("arialbd.ttf" if bold else "arial.ttf")), size)


def vertical_gradient(size, top, bottom):
    image = Image.new("RGB", size, top)
    draw = ImageDraw.Draw(image)
    width, height = size
    for y in range(height):
        t = y / max(1, height - 1)
        colour = tuple(int(top[i] + (bottom[i] - top[i]) * t) for i in range(3))
        draw.line([(0, y), (width, y)], fill=colour)
    return image


def draw_wifi(draw, cx, cy, scale, colour):
    for i in range(3):
        r = (10 + i * 8) * scale
        draw.arc([cx - r, cy + 14 * scale - r, cx + r, cy + 14 * scale + r], 225, 315, fill=colour,
                 width=max(2, int(4 * scale)))
    d = 3 * scale
    draw.ellipse([cx - d, cy + 12 * scale - d, cx + d, cy + 12 * scale + d], fill=colour)


def draw_screen_mockup(base, x, y, scale):
    """The remote screen of the firmware (240x135 px) in Wi-Fi mode, scaled."""
    w, h = int(240 * scale), int(135 * scale)
    screen = Image.new("RGB", (w, h), BACKGROUND)
    d = ImageDraw.Draw(screen)
    s = scale

    # Status bar: WI-FI chip, dot, TV name, battery.
    d.rectangle([0, 0, w, 18 * s], fill=SURFACE)
    d.rounded_rectangle([2 * s, 2 * s, 44 * s, 16 * s], radius=4 * s, fill=MODES[0][1])
    d.text((23 * s, 9 * s), "WI-FI", font=font(int(8 * s), True), fill=TEXT, anchor="mm")
    d.ellipse([52 * s - 3 * s, 9 * s - 3 * s, 52 * s + 3 * s, 9 * s + 3 * s], fill=OK)
    d.text((59 * s, 9 * s), "Living room TV", font=font(int(8 * s)), fill=TEXT, anchor="lm")
    d.rectangle([w - 24 * s, 5 * s, w - 6 * s, 13 * s], outline=TEXT, width=max(1, int(s)))
    d.rectangle([w - 22 * s, 7 * s, w - 10 * s, 11 * s], fill=TEXT)
    d.text((w - 27 * s, 9 * s), "87%", font=font(int(7 * s)), fill=MUTED, anchor="rm")

    # Tinted content, band, badge, key caps, footer.
    d.rectangle([0, 18 * s, w, 119 * s], fill=TINT_WIFI)
    d.rectangle([0, 18 * s, w, 42 * s], fill=MODES[0][1])
    draw_wifi(d, 16 * s, 26 * s, 0.45 * s, TEXT)
    d.text((32 * s, 30 * s), "WI-FI REMOTE", font=font(int(11 * s), True), fill=TEXT, anchor="lm")

    cy = 68 * s
    d.ellipse([24 * s - 13 * s, cy - 13 * s, 24 * s + 13 * s, cy + 13 * s], fill=OK)
    d.line([(18 * s, cy), (22 * s, cy + 5 * s), (30 * s, cy - 5 * s)], fill=TEXT, width=max(2, int(3 * s)))
    d.text((46 * s, cy - 8 * s), "Connected", font=font(int(11 * s), True), fill=TEXT, anchor="lm")
    d.text((46 * s, cy + 8 * s), "Living room TV", font=font(int(9 * s)), fill=MUTED, anchor="lm")

    keys = [("TAB", "Help"), ("OPT", "Menu"), ("A", "Air mouse")]
    kx = 20 * s
    for key, label in keys:
        kf, lf = font(int(7 * s), True), font(int(8 * s))
        cap_w = max(16 * s, d.textlength(key, font=kf) + 8 * s)
        d.rounded_rectangle([kx, 100 * s, kx + cap_w, 113 * s], radius=3 * s, fill=TEXT)
        d.text((kx + cap_w / 2, 106 * s), key, font=kf, fill=BACKGROUND, anchor="mm")
        d.text((kx + cap_w + 4 * s, 106.5 * s), label, font=lf, fill=TEXT, anchor="lm")
        kx += cap_w + 4 * s + d.textlength(label, font=lf) + 18 * s

    d.rectangle([0, 119 * s, w, h], fill=SURFACE)
    d.text((w / 2, 127 * s), "t type  l apps  p power  0-9 digits", font=font(int(7 * s)), fill=MUTED, anchor="mm")

    # Device bezel around the screen.
    bezel = int(16 * scale / 2.2)
    frame = Image.new("RGB", (w + 2 * bezel, h + 2 * bezel), (196, 192, 186))
    frame_draw = ImageDraw.Draw(frame)
    frame_draw.rounded_rectangle([0, 0, frame.width - 1, frame.height - 1], radius=bezel, fill=(196, 192, 186))
    frame_draw.rectangle([bezel - 3, bezel - 3, bezel + w + 2, bezel + h + 2], fill=(40, 40, 44))
    frame.paste(screen, (bezel, bezel))
    base.paste(frame, (x, y))
    return frame.size


def draw_mode_chips(draw, x, y, size):
    f = font(size, True)
    for name, colour in MODES:
        width = draw.textlength(name, font=f) + size * 1.2
        draw.rounded_rectangle([x, y, x + width, y + size * 1.8], radius=size * 0.9, fill=colour)
        draw.text((x + width / 2, y + size * 0.9), name, font=f, fill=TEXT, anchor="mm")
        x += width + size * 0.6


def wide_cover():
    image = vertical_gradient((1280, 720), (16, 20, 40), (6, 6, 10))
    draw = ImageDraw.Draw(image)
    draw.rectangle([0, 0, 1280, 10], fill=ACCENT)

    draw.text((74, 110), "cardputer.ip9.pl", font=font(26, True), fill=ACCENT)
    draw.text((70, 160), "IP9 LG Remote", font=font(74, True), fill=TEXT)
    draw.text((74, 258), "for M5Stack Cardputer & ADV", font=font(32), fill=MUTED)
    draw_mode_chips(draw, 74, 330, 28)
    draw.text((74, 430), "Pointer · Typing · Air mouse · Apps", font=font(28), fill=TEXT)
    draw.text((74, 478), "webOS Wi-Fi pairing, IR for every LG TV", font=font(24), fill=MUTED)
    draw.text((74, 640), "Open source · MIT · Patryk Strzelecki · cardputer.ip9.pl", font=font(22), fill=MUTED)

    draw_screen_mockup(image, 690, 200, 2.2)
    image.save(OUT / "cover.png", optimize=True)


def square_cover():
    image = vertical_gradient((800, 800), (16, 20, 40), (6, 6, 10))
    draw = ImageDraw.Draw(image)
    draw.rectangle([0, 0, 800, 10], fill=ACCENT)
    draw.text((400, 95), "IP9 LG Remote", font=font(76, True), fill=TEXT, anchor="mm")
    draw.text((400, 165), "for M5Stack Cardputer & ADV", font=font(32), fill=MUTED, anchor="mm")
    size = draw_screen_mockup(Image.new("RGB", (1, 1)), 0, 0, 2.6)  # measure the framed mock-up
    draw_screen_mockup(image, (800 - size[0]) // 2, 230, 2.6)
    # Centre the chips row.
    f_size = 28
    total = sum(draw.textlength(n, font=font(f_size, True)) + f_size * 1.2 for n, _ in MODES) + f_size * 0.6 * 3
    draw_mode_chips(draw, (800 - total) / 2, 640, f_size)
    draw.text((400, 740), "Pointer · Typing · Air mouse · Apps", font=font(26), fill=TEXT, anchor="mm")
    draw.text((400, 778), "cardputer.ip9.pl", font=font(20, True), fill=ACCENT, anchor="mm")
    image.save(OUT / "cover-square.png", optimize=True)


if __name__ == "__main__":
    OUT.mkdir(exist_ok=True)
    wide_cover()
    square_cover()
    print(f"Written {OUT / 'cover.png'} and {OUT / 'cover-square.png'}")
