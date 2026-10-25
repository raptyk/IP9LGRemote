// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#include "StatusLed.h"

#include <M5Unified.h>
#include <math.h>

namespace lgremote {

namespace {
constexpr uint32_t kBreathPeriodMs = 2400;
constexpr uint8_t kSteadyLevel = 40;  // pointer mode: a gentle glow, not a torch
// A lit LED is re-sent regularly so it recovers by itself if its supply dipped
// (on the ADV the supply is switched together with the display backlight).
constexpr uint32_t kRefreshMs = 50;
}  // namespace

void StatusLed::begin() {
    // Driven with the Arduino core's neopixelWrite() (RMT, 24-bit GRB, NeoPixel timing).
    // M5.Led cannot be used: M5Unified's RMT LED bus is empty on ESP-IDF 4.x (Arduino core 2.x).
    pin_ = M5.getPin(m5::pin_name_t::rgb_led);
    if (pin_ >= 0) {
        show(0, 0);
    }
}

void StatusLed::setEnabled(bool enabled) {
    enabled_ = enabled;
    if (!enabled_) {
        burstCount_ = 0;
        sequenceLength_ = 0;
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

void StatusLed::playSequence(const uint32_t* colours, size_t count, uint16_t stepMs) {
    sequenceLength_ = count < kMaxSequence ? count : kMaxSequence;
    for (size_t i = 0; i < sequenceLength_; ++i) {
        sequence_[i] = colours[i];
    }
    sequenceStepMs_ = stepMs;
    sequenceStartedAt_ = millis();
}

bool StatusLed::wantsPower() const {
    return pin_ >= 0 && enabled_ && (sequenceLength_ > 0 || burstCount_ > 0 || pattern_ != LedPattern::Off);
}

void StatusLed::loop() {
    if (pin_ < 0 || !enabled_) {
        return;
    }
    const uint32_t now = millis();

    // Sequences and one-shot signals win over the background pattern.
    if (sequenceLength_ > 0) {
        const size_t step = (now - sequenceStartedAt_) / sequenceStepMs_;
        if (step < sequenceLength_) {
            show(sequence_[step], kMaxLevel);
            return;
        }
        sequenceLength_ = 0;
    }
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
            const uint8_t level = static_cast<uint8_t>(8 + wave * (kMaxLevel / 2)) & 0xFC;
            show(patternColour_, level);
            break;
        }
    }
}

void StatusLed::show(uint32_t colour, uint8_t level) {
    if (pin_ < 0) {
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
    neopixelWrite(static_cast<uint8_t>(pin_), red, green, blue);
}

}  // namespace lgremote
