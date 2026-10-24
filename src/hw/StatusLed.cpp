// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#include "StatusLed.h"

#include <M5Unified.h>
#include <math.h>

namespace lgremote {

namespace {
constexpr uint32_t kBreathPeriodMs = 2400;
constexpr uint8_t kSteadyLevel = 24;  // pointer mode: a gentle glow, not a torch
}  // namespace

void StatusLed::begin() {
    available_ = M5.Led.isEnabled() && M5.Led.begin();
    if (available_) {
        M5.Led.setBrightness(255);  // levels are applied per colour in show()
        show(0, 0);
    }
}

void StatusLed::setEnabled(bool enabled) {
    enabled_ = enabled;
    if (!enabled_) {
        burstCount_ = 0;
        show(0, 0);
    }
}

void StatusLed::setBackground(LedPattern pattern, uint32_t colour) {
    pattern_ = pattern;
    patternColour_ = colour;
}

void StatusLed::flash(uint32_t colour, uint16_t durationMs) {
    blink(colour, 1, durationMs, 0);
}

void StatusLed::blink(uint32_t colour, uint8_t count, uint16_t onMs, uint16_t offMs) {
    burstColour_ = colour;
    burstCount_ = count;
    burstOnMs_ = onMs;
    burstOffMs_ = offMs;
    burstStartedAt_ = millis();
}

void StatusLed::loop() {
    if (!available_ || !enabled_) {
        return;
    }
    const uint32_t now = millis();

    // One-shot signals win over the background pattern.
    if (burstCount_ > 0) {
        const uint32_t cycle = burstOnMs_ + burstOffMs_;
        const uint32_t elapsed = now - burstStartedAt_;
        if (elapsed < cycle * burstCount_) {
            const bool on = (elapsed % cycle) < burstOnMs_;
            show(burstColour_, on ? kMaxLevel : 0);
            return;
        }
        burstCount_ = 0;
    }

    switch (pattern_) {
        case LedPattern::Off:
            show(0, 0);
            break;
        case LedPattern::Steady:
            show(patternColour_, kSteadyLevel);
            break;
        case LedPattern::Breathing: {
            const float phase = static_cast<float>(now % kBreathPeriodMs) / kBreathPeriodMs;
            const float wave = 0.5f - 0.5f * cosf(phase * 2.0f * PI);
            // Quantised to steps of 4 so the LED is not rewritten on every loop iteration.
            const uint8_t level = static_cast<uint8_t>(wave * (kMaxLevel / 2)) & 0xFC;
            show(patternColour_, level);
            break;
        }
    }
}

void StatusLed::show(uint32_t colour, uint8_t level) {
    if (!available_ || (colour == shownColour_ && level == shownLevel_)) {
        return;
    }
    shownColour_ = colour;
    shownLevel_ = level;
    const uint8_t red = ((colour >> 16) & 0xFF) * level / 255;
    const uint8_t green = ((colour >> 8) & 0xFF) * level / 255;
    const uint8_t blue = (colour & 0xFF) * level / 255;
    M5.Led.setAllColor(red, green, blue);
    M5.Led.display();
}

}  // namespace lgremote
