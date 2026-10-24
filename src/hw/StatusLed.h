// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

#include <stdint.h>

namespace lgremote {

// Background pattern of the RGB status LED.
enum class LedPattern : uint8_t {
    Off,        // idle and connected: dark, friendly for a dim living room and the battery
    Breathing,  // waiting for something (Wi-Fi, pairing, Bluetooth, USB host)
    Steady      // a mode is active (pointer)
};

// WS2812 RGB LED (GPIO21 on Cardputer and Cardputer ADV) driven through M5Unified.
// A background pattern plus short one-shot signals (flashes, blink bursts) on top of it.
// Colours are 0xRRGGBB.
class StatusLed {
public:
    void begin();
    void setEnabled(bool enabled);
    bool enabled() const { return enabled_; }

    void setBackground(LedPattern pattern, uint32_t colour);

    // One short flash, e.g. "command sent".
    void flash(uint32_t colour, uint16_t durationMs = 60);
    // A burst of `count` blinks, e.g. two green = connected, three red = connection lost.
    void blink(uint32_t colour, uint8_t count, uint16_t onMs = 90, uint16_t offMs = 110);

    void loop();

private:
    static constexpr uint8_t kMaxLevel = 64;  // of 255: visible in daylight, not glaring at night

    void show(uint32_t colour, uint8_t level);

    bool available_ = false;
    bool enabled_ = true;
    LedPattern pattern_ = LedPattern::Off;
    uint32_t patternColour_ = 0;

    uint32_t burstColour_ = 0;
    uint8_t burstCount_ = 0;
    uint16_t burstOnMs_ = 0;
    uint16_t burstOffMs_ = 0;
    uint32_t burstStartedAt_ = 0;

    uint32_t shownColour_ = 0xFFFFFFFF;  // forces the first update
    uint8_t shownLevel_ = 0xFF;
};

}  // namespace lgremote
