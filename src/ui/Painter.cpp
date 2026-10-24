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
    drawClippedText(canvas, title, kPadding, top + 2, kScreenWidth - 2 * kPadding);
    canvas.drawFastHLine(kPadding, top + 18, kScreenWidth - 2 * kPadding, kSurfaceHigh);
    return top + 20;
}

void drawListRow(M5Canvas& canvas, int y, int height, const String& label, const String& value, bool selected,
                 bool enabled) {
    if (selected) {
        canvas.fillRoundRect(3, y, kScreenWidth - 10, height, 4, enabled ? kAccent : kSurfaceHigh);
    }
    const uint16_t labelColour = enabled ? kText : (selected ? kTextMuted : kTextDisabled);
    canvas.setFont(&fonts::FreeSans9pt7b);
    canvas.setTextSize(1);
    canvas.setTextColor(labelColour);
    const int centerY = y + height / 2 + 1;

    int labelWidth = kScreenWidth - 2 * kPadding - 8;
    if (value.length() > 0) {
        canvas.setFont(&fonts::Font0);
        const int valueWidth = min<int>(canvas.textWidth(value), 110);
        canvas.setTextColor(selected ? kText : kTextMuted);
        canvas.setTextDatum(middle_right);
        drawClippedText(canvas, value, kScreenWidth - kPadding - 6, centerY, valueWidth);
        labelWidth -= valueWidth + 8;
        canvas.setFont(&fonts::FreeSans9pt7b);
        canvas.setTextColor(labelColour);
    }
    canvas.setTextDatum(middle_left);
    drawClippedText(canvas, label, kPadding + 2, centerY, labelWidth);
}

std::vector<String> wrapText(M5Canvas& canvas, const String& text, int maxWidth) {
    std::vector<String> lines;
    String line;
    int start = 0;
    const int length = text.length();
    while (start <= length) {
        int end = text.indexOf(' ', start);
        if (end < 0) {
            end = length;
        }
        const String word = text.substring(start, end);
        const String candidate = line.isEmpty() ? word : line + " " + word;
        if (canvas.textWidth(candidate) <= maxWidth || line.isEmpty()) {
            line = candidate;
        } else {
            lines.push_back(line);
            line = word;
        }
        start = end + 1;
    }
    if (!line.isEmpty()) {
        lines.push_back(line);
    }
    return lines;
}

int drawWrappedText(M5Canvas& canvas, const String& text, int x, int y, int maxWidth, int lineHeight, int maxLines) {
    std::vector<String> lines = wrapText(canvas, text, maxWidth);
    const int count = min<int>(lines.size(), maxLines);
    for (int i = 0; i < count; ++i) {
        String line = lines[i];
        if (i == count - 1 && static_cast<int>(lines.size()) > count) {
            line += " ...";  // more text than room: show that something is cut
        }
        drawClippedText(canvas, line, x, y + i * lineHeight, maxWidth);
    }
    return count * lineHeight;
}

void drawCenteredLines(M5Canvas& canvas, int top, int height, const char* title, const char* body,
                       const char* detail) {
    constexpr int kTitleHeight = 20;
    constexpr int kBodyLineHeight = 15;
    const int maxWidth = kScreenWidth - 2 * kPadding;

    // Measure first so the block can be centred vertically.
    canvas.setTextSize(1);
    canvas.setFont(&fonts::Font2);
    std::vector<String> bodyLines;
    for (const char* part : {body, detail}) {
        if (part != nullptr && part[0] != '\0') {
            for (const String& line : wrapText(canvas, part, maxWidth)) {
                bodyLines.push_back(line);
            }
        }
    }
    const int maxBodyLines = max(0, (height - kTitleHeight) / kBodyLineHeight);
    const int bodyCount = min<int>(bodyLines.size(), maxBodyLines);
    const int blockHeight = (title != nullptr ? kTitleHeight : 0) + bodyCount * kBodyLineHeight;
    int y = top + (height - blockHeight) / 2;

    canvas.setTextDatum(top_center);
    if (title != nullptr) {
        canvas.setFont(&fonts::FreeSansBold9pt7b);
        canvas.setTextColor(kText);
        drawClippedText(canvas, title, kScreenWidth / 2, y + 2, maxWidth);
        y += kTitleHeight;
    }
    canvas.setFont(&fonts::Font2);
    canvas.setTextColor(kTextMuted);
    for (int i = 0; i < bodyCount; ++i) {
        drawClippedText(canvas, bodyLines[i], kScreenWidth / 2, y, maxWidth);
        y += kBodyLineHeight;
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
