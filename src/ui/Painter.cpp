// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#include "Painter.h"

#include "Theme.h"

namespace lgremote::painter {

using namespace theme;

void drawClippedText(M5Canvas& canvas, const String& text, int x, int y, int maxWidth) {
    if (canvas.textWidth(text) <= maxWidth) {
        canvas.drawString(text, x, y);
        return;
    }
    String clipped = text;
    while (clipped.length() > 0 && canvas.textWidth(clipped + "...") > maxWidth) {
        clipped.remove(clipped.length() - 1);
    }
    canvas.drawString(clipped + "...", x, y);
}

void drawBattery(M5Canvas& canvas, int right, int centerY, int percent, bool charging) {
    const int bodyWidth = 18;
    const int bodyHeight = 9;
    const int x = right - bodyWidth - 2;
    const int y = centerY - bodyHeight / 2;
    const uint16_t colour = percent < 0 ? kTextMuted : (percent <= 15 ? kError : (percent <= 35 ? kWarn : kText));

    canvas.drawRect(x, y, bodyWidth, bodyHeight, colour);
    canvas.fillRect(x + bodyWidth, y + 2, 2, bodyHeight - 4, colour);
    if (percent >= 0) {
        const int fill = (bodyWidth - 4) * constrain(percent, 0, 100) / 100;
        canvas.fillRect(x + 2, y + 2, fill, bodyHeight - 4, colour);
    }
    if (charging) {
        // Lightning bolt drawn over the body.
        const int cx = x + bodyWidth / 2;
        canvas.fillTriangle(cx + 1, y - 1, cx - 3, centerY + 1, cx, centerY + 1, kWarn);
        canvas.fillTriangle(cx - 1, y + bodyHeight + 1, cx + 3, centerY - 1, cx, centerY - 1, kWarn);
    }

    canvas.setFont(&fonts::Font0);
    canvas.setTextSize(1);
    canvas.setTextColor(kTextMuted);
    canvas.setTextDatum(middle_right);
    if (percent >= 0) {
        canvas.drawString(String(percent) + "%", x - 3, centerY);
    }
}

int drawTitle(M5Canvas& canvas, int top, const char* title) {
    canvas.setFont(&fonts::FreeSansBold9pt7b);
    canvas.setTextSize(1);
    canvas.setTextColor(kText);
    canvas.setTextDatum(top_left);
    canvas.drawString(title, kPadding, top + 3);
    canvas.drawFastHLine(kPadding, top + 21, kScreenWidth - 2 * kPadding, kSurfaceHigh);
    return top + 24;
}

void drawListRow(M5Canvas& canvas, int y, int height, const String& label, const String& value, bool selected) {
    if (selected) {
        canvas.fillRoundRect(3, y, kScreenWidth - 10, height, 4, kAccent);
    }
    canvas.setFont(&fonts::FreeSans9pt7b);
    canvas.setTextSize(1);
    canvas.setTextColor(kText);
    const int centerY = y + height / 2 + 1;

    int labelWidth = kScreenWidth - 2 * kPadding - 8;
    if (value.length() > 0) {
        canvas.setFont(&fonts::Font0);
        const int valueWidth = min<int>(canvas.textWidth(value), 90);
        canvas.setTextColor(selected ? kText : kTextMuted);
        canvas.setTextDatum(middle_right);
        drawClippedText(canvas, value, kScreenWidth - kPadding - 6, centerY, valueWidth);
        labelWidth -= valueWidth + 8;
        canvas.setFont(&fonts::FreeSans9pt7b);
        canvas.setTextColor(kText);
    }
    canvas.setTextDatum(middle_left);
    drawClippedText(canvas, label, kPadding + 2, centerY, labelWidth);
}

void drawCenteredLines(M5Canvas& canvas, int top, int height, const char* line1, const char* line2,
                       const char* line3) {
    const char* lines[] = {line1, line2, line3};
    int count = 0;
    for (const char* line : lines) {
        if (line != nullptr) {
            ++count;
        }
    }
    const int lineHeight = 18;
    int y = top + (height - count * lineHeight) / 2 + lineHeight / 2;
    canvas.setTextSize(1);
    canvas.setTextDatum(middle_center);
    for (int i = 0; i < 3; ++i) {
        if (lines[i] == nullptr) {
            continue;
        }
        canvas.setFont(i == 0 ? &fonts::FreeSansBold9pt7b : &fonts::FreeSans9pt7b);
        canvas.setTextColor(i == 0 ? kText : kTextMuted);
        drawClippedText(canvas, lines[i], kScreenWidth / 2, y, kScreenWidth - 2 * kPadding);
        y += lineHeight;
    }
}

void drawScrollBar(M5Canvas& canvas, int top, int height, int first, int visible, int total) {
    if (total <= visible) {
        return;
    }
    const int x = kScreenWidth - 4;
    canvas.fillRect(x, top, 2, height, kSurface);
    const int thumbHeight = max(8, height * visible / total);
    const int thumbTop = top + (height - thumbHeight) * first / max(1, total - visible);
    canvas.fillRect(x, thumbTop, 2, thumbHeight, kTextMuted);
}

void drawSpinner(M5Canvas& canvas, int x, int y) {
    const int phase = (millis() / 250) % 3;
    for (int i = 0; i < 3; ++i) {
        canvas.fillCircle(x + (i - 1) * 12, y, 3, i == phase ? kAccent : kSurfaceHigh);
    }
}

}  // namespace lgremote::painter
