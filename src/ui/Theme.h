// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

#include <stdint.h>

namespace lgremote::theme {

// RGB565 palette. Dark background keeps the screen readable in a dim living room
// and saves battery; LG magenta accent echoes the Magic Remote branding colour.
constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
    return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

constexpr uint16_t kBackground = rgb(10, 10, 14);
constexpr uint16_t kSurface = rgb(28, 28, 36);
constexpr uint16_t kSurfaceHigh = rgb(48, 48, 60);
constexpr uint16_t kText = rgb(236, 236, 240);
constexpr uint16_t kTextMuted = rgb(140, 140, 155);
constexpr uint16_t kAccent = rgb(230, 0, 110);
constexpr uint16_t kOk = rgb(60, 200, 110);
constexpr uint16_t kWarn = rgb(245, 170, 40);
constexpr uint16_t kError = rgb(235, 70, 60);
constexpr uint16_t kKeyRed = rgb(220, 50, 50);
constexpr uint16_t kKeyGreen = rgb(50, 180, 70);
constexpr uint16_t kKeyYellow = rgb(230, 200, 40);
constexpr uint16_t kKeyBlue = rgb(50, 110, 230);
constexpr uint16_t kTextDisabled = rgb(80, 80, 92);

// One colour per link mode, used by the status bar chip so the mode is obvious at a glance.
constexpr uint16_t kModeWifi = rgb(30, 120, 230);
constexpr uint16_t kModeIr = rgb(225, 80, 30);
constexpr uint16_t kModeBluetooth = rgb(110, 80, 220);
constexpr uint16_t kModeUsb = rgb(30, 160, 90);

// Dark tints of the mode colours: background of the remote screen, so it never looks like a menu.
constexpr uint16_t kTintWifi = rgb(8, 22, 46);
constexpr uint16_t kTintIr = rgb(42, 16, 6);
constexpr uint16_t kTintBluetooth = rgb(24, 16, 48);
constexpr uint16_t kTintUsb = rgb(6, 32, 18);

constexpr int kScreenWidth = 240;
constexpr int kScreenHeight = 135;
constexpr int kStatusBarHeight = 18;
constexpr int kFooterHeight = 16;
constexpr int kPadding = 6;

}  // namespace lgremote::theme
