// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

#include <M5GFX.h>

#include <vector>

namespace lgremote::painter {

// Draws text at (x, y) with the current datum, truncating with "..." to fit `maxWidth`.
void drawClippedText(M5Canvas& canvas, const String& text, int x, int y, int maxWidth);

// Battery icon right-aligned at `right`, vertically centred on `centerY`.
void drawBattery(M5Canvas& canvas, int right, int centerY, int percent, bool charging);

// Screen title line at the top of the content area; returns the y where content starts.
int drawTitle(M5Canvas& canvas, int top, const char* title);

// One list row; `selected` rows are highlighted.
// Disabled rows are greyed out (and highlighted in grey instead of the accent colour).
void drawListRow(M5Canvas& canvas, int y, int height, const String& label, const String& value, bool selected,
                 bool enabled = true);

// Splits text into lines that fit `maxWidth` with the canvas' current font (word wrap).
std::vector<String> wrapText(M5Canvas& canvas, const String& text, int maxWidth);

// Word-wrapped text with the current font and datum; returns the height used.
int drawWrappedText(M5Canvas& canvas, const String& text, int x, int y, int maxWidth, int lineHeight, int maxLines);

// Bold title plus word-wrapped body text (smaller font), centred in the given area.
void drawCenteredLines(M5Canvas& canvas, int top, int height, const char* title, const char* body = nullptr,
                       const char* detail = nullptr);

// Small vertical scroll indicator on the right edge.
void drawScrollBar(M5Canvas& canvas, int top, int height, int first, int visible, int total);

// Busy indicator (three pulsing dots) centred at (x, y).
void drawSpinner(M5Canvas& canvas, int x, int y);

}  // namespace lgremote::painter
