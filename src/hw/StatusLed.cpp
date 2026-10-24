// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#include "StatusLed.h"

#include <M5Unified.h>
#include <esp32-hal-rmt.h>
#include <math.h>

namespace lgremote {

namespace {
constexpr uint32_t kBreathPeriodMs = 2400;
constexpr uint8_t kSteadyLevel = 40;  // pointer mode: a gentle glow, not a torch
// On the ADV the LED supply follows the PWM backlight; a lit LED is re-sent regularly so
// it recovers when the screen dims or wakes.
constexpr uint32_t kRefreshMs = 250;

// NeoPixel timing with a 100 ns RMT tick (same as the core's neopixelWrite()).
constexpr float kRmtTickNs = 100;
constexpr uint32_t kBitHighTicks = 8;  // "1": 0.8 us high, 0.4 us low
constexpr uint32_t kBitLowTicks = 4;   // "0": 0.4 us high, 0.8 us low
constexpr size_t kFrameBits = 32;      // G, R, B and an unused W byte
}  // namespace

void StatusLed::begin() {
    // The LED is driven with the Arduino core RMT driver, like the core's neopixelWrite():
    // M5.Led cannot be used because M5Unified's RMT LED bus is empty on ESP-IDF 4.x
    // (Arduino core 2.x), so it never sends anything there.
    // Frames carry 32 bits (GRB + an unused white byte): a 3-byte NeoPixel takes the first
    // 24 bits and passes the rest on, an RGBW variant gets the full frame it expects.
    const int pin = M5.getPin(m5::pin_name_t::rgb_led);
    if (pin < 0) {
        return;
    }
    rmt_ = rmtInit(pin, RMT_TX_MODE, RMT_MEM_64);
    if (rmt_ == nullptr) {
        log_e("Status LED: RMT init failed on GPIO%d", pin);
        return;
    }
    rmtSetTick(rmt_, kRmtTickNs);
    available_ = true;
    show(0, 0);
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
    const uint8_t bytes[4] = {green, red, blue, 0};  // GRB(W) order, white channel unused

    rmt_data_t frame[kFrameBits];
    for (size_t i = 0; i < kFrameBits; ++i) {
        const bool one = (bytes[i / 8] >> (7 - i % 8)) & 0x01;
        frame[i].level0 = 1;
        frame[i].duration0 = one ? kBitHighTicks : kBitLowTicks;
        frame[i].level1 = 0;
        frame[i].duration1 = one ? kBitLowTicks : kBitHighTicks;
    }
    rmtWriteBlocking(rmt_, frame, kFrameBits);
}

}  // namespace lgremote
