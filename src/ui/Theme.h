// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

#include "ip9/ui/Theme.h"

namespace lgremote::theme {

// The shared IP9 palette (ip9::theme) plus the colours of this app.
using namespace ip9::theme;

// LG magenta accent echoes the Magic Remote branding colour.
constexpr uint16_t kAccent = rgb(230, 0, 110);
constexpr uint16_t kKeyRed = rgb(220, 50, 50);
constexpr uint16_t kKeyGreen = rgb(50, 180, 70);
constexpr uint16_t kKeyYellow = rgb(230, 200, 40);
constexpr uint16_t kKeyBlue = rgb(50, 110, 230);

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

}  // namespace lgremote::theme
