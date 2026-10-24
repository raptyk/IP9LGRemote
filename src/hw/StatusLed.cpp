// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#include "StatusLed.h"

#include <M5Unified.h>
#include <math.h>
#include <utility/led/LED_Strip_Class.hpp>

namespace lgremote {

namespace {
constexpr uint32_t kBreathPeriodMs = 2400;
constexpr uint8_t kSteadyLevel = 24;  // pointer mode: a gentle glow, not a torch
// On the ADV the LED supply follows the PWM backlight; a lit LED is re-sent regularly so
// it recovers when the screen dims or wakes.
constexpr uint32_t kRefreshMs = 250;
}  // namespace

StatusLed::StatusLed() = default;
StatusLed::~StatusLed() = default;

void StatusLed::begin() {
    // M5.Led only sends 3 bytes per LED (WS2812). The Cardputer ADV carries an SK6812 RGBW
    // LED that needs 4 bytes (G, R, B, W) and ignores shorter frames, so the frame is sent
    // directly over M5Unified's RMT bus. A 3-byte WS2812 (original Cardputer) takes the first
    // 24 bits - the same colour - and passes the extra byte on, so this works on both.
    // No power pin: on the ADV the LED supply follows GPIO38, which is the display backlight.
    const int pin = M5.getPin(m5::pin_name_t::rgb_led);
    if (pin < 0) {
        return;
    }
    bus_ = std::make_unique<m5::LedBus_RMT>();
    auto config = bus_->getConfig();
    config.pin_data = static_cast<int8_t>(pin);
    config.pin_power = -1;
    bus_->setConfig(config);
    available_ = bus_->init();
    if (available_) {
        show(0, 0);
    } else {
        bus_.reset();
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
    if (!available_) {
        return;
    }
    const uint32_t now = millis();
    const bool unchanged = colour == shownColour_ && level == shownLevel_;
    if (unchanged && (level == 0 || now - lastWriteAt_ < kRefreshMs)) {
        return;
    }
    shownColour_ = colour;
    shownLevel_ = level;
    lastWriteAt_ = now;
    const uint8_t red = ((colour >> 16) & 0xFF) * level / 255;
    const uint8_t green = ((colour >> 8) & 0xFF) * level / 255;
    const uint8_t blue = (colour & 0xFF) * level / 255;
    const uint8_t frame[4] = {green, red, blue, 0};  // GRB(W) order, white channel unused
    bus_->write(frame, sizeof(frame));
}

}  // namespace lgremote
