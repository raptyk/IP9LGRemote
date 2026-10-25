// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#include "Icons.h"

#include <math.h>

#include "Theme.h"
#include "ip9/ui/Icons.h"

namespace lgremote::icons {

namespace {

int s(float value, float scale) {
    return ip9::icons::scaled(value, scale);
}

void thickLine(M5Canvas& canvas, int x0, int y0, int x1, int y1, int width, uint16_t colour) {
    canvas.drawWideLine(x0, y0, x1, y1, width / 2.0f, colour);
}

void drawWifi(M5Canvas& canvas, int cx, int cy, uint16_t colour, float k) {
    const int baseY = cy + s(14, k);
    for (int i = 0; i < 3; ++i) {
        const int radius = s(10 + i * 8, k);
        const int width = max(2, s(4, k));
        canvas.fillArc(cx, baseY, radius, radius - width, 225, 315, colour);
    }
    canvas.fillCircle(cx, baseY - s(2, k), max(2, s(3, k)), colour);
}

void drawInfrared(M5Canvas& canvas, int cx, int cy, uint16_t colour, float k) {
    // Remote body with buttons, signal waves above.
    const int bodyWidth = s(14, k);
    const int bodyHeight = s(24, k);
    const int bodyX = cx - bodyWidth / 2;
    const int bodyY = cy - s(4, k);
    canvas.drawRoundRect(bodyX, bodyY, bodyWidth, bodyHeight, s(3, k), colour);
    canvas.drawRoundRect(bodyX + 1, bodyY + 1, bodyWidth - 2, bodyHeight - 2, s(3, k), colour);
    canvas.fillCircle(cx, bodyY + s(6, k), max(1, s(2, k)), theme::kAccent);
    for (int row = 0; row < 2; ++row) {
        for (int column = -1; column <= 1; column += 2) {
            canvas.fillRect(cx + column * s(3, k) - 1, bodyY + s(12 + row * 5, k), max(2, s(2, k)), max(2, s(2, k)),
                            colour);
        }
    }
    for (int i = 0; i < 2; ++i) {
        const int radius = s(9 + i * 6, k);
        canvas.fillArc(cx, bodyY - s(2, k), radius, radius - max(2, s(2, k)), 240, 300, colour);
    }
}

void drawBluetooth(M5Canvas& canvas, int cx, int cy, uint16_t colour, float k) {
    const int top = cy - s(16, k);
    const int bottom = cy + s(16, k);
    const int right = cx + s(9, k);
    const int left = cx - s(9, k);
    const int width = max(2, s(3, k));
    thickLine(canvas, cx, top, cx, bottom, width, colour);
    thickLine(canvas, cx, top, right, cy - s(8, k), width, colour);
    thickLine(canvas, right, cy - s(8, k), left, cy + s(8, k), width, colour);
    thickLine(canvas, cx, bottom, right, cy + s(8, k), width, colour);
    thickLine(canvas, right, cy + s(8, k), left, cy - s(8, k), width, colour);
}

void drawUsb(M5Canvas& canvas, int cx, int cy, uint16_t colour, float k) {
    // USB-C plug outline with cable.
    const int plugWidth = s(26, k);
    const int plugHeight = s(12, k);
    canvas.fillRoundRect(cx - plugWidth / 2, cy - s(14, k), plugWidth, plugHeight, plugHeight / 2, colour);
    canvas.fillRoundRect(cx - plugWidth / 2 + s(5, k), cy - s(10, k), plugWidth - s(10, k), max(2, s(4, k)),
                         max(1, s(2, k)), theme::kBackground);
    canvas.fillRect(cx - s(8, k), cy - s(2, k), s(16, k), s(10, k), colour);
    thickLine(canvas, cx, cy + s(8, k), cx, cy + s(18, k), max(2, s(4, k)), colour);
}

}  // namespace

void draw(M5Canvas& canvas, Icon icon, int cx, int cy, uint16_t colour, float scale, uint16_t background) {
    switch (icon) {
        case Icon::Wifi: drawWifi(canvas, cx, cy, colour, scale); break;
        case Icon::Infrared: drawInfrared(canvas, cx, cy, colour, scale); break;
        case Icon::Bluetooth: drawBluetooth(canvas, cx, cy, colour, scale); break;
        case Icon::Usb: drawUsb(canvas, cx, cy, colour, scale); break;
        case Icon::Settings: ip9::icons::drawSettings(canvas, cx, cy, colour, scale, background); break;
        case Icon::Help: ip9::icons::drawHelp(canvas, cx, cy, colour, scale); break;
    }
}

}  // namespace lgremote::icons
