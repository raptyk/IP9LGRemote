// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

#include <M5GFX.h>

namespace lgremote::icons {

enum class Icon : uint8_t { Wifi, Infrared, Bluetooth, Usb, Settings, Help };

// Draws a 40x40 vector icon centred at (cx, cy). `scale` 1.0 = 40 px, 0.5 = 20 px.
void draw(M5Canvas& canvas, Icon icon, int cx, int cy, uint16_t colour, float scale = 1.0f);

}  // namespace lgremote::icons
