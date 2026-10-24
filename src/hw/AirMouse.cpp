// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#include "AirMouse.h"

#include <M5Cardputer.h>

#include <math.h>

namespace lgremote {

namespace {

// Pixels per degree for speed levels 1..5.
constexpr float kGain[] = {0.25f, 0.4f, 0.6f, 0.85f, 1.2f};

// Slight acceleration: slow wrist movements are precise, fast ones cross the screen.
float shape(float rateDps, float deadZoneDps) {
    float magnitude = fabsf(rateDps) - deadZoneDps;
    if (magnitude <= 0) {
        return 0;
    }
    float shaped = powf(magnitude, 1.25f);
    return rateDps < 0 ? -shaped : shaped;
}

}  // namespace

void AirMouse::begin(bool available) {
    available_ = available;
}

void AirMouse::start() {
    if (!available_) {
        return;
    }
    active_ = true;
    calibrating_ = true;
    calibrationSamples_ = 0;
    biasX_ = 0;
    biasZ_ = 0;
    accX_ = 0;
    accY_ = 0;
    startedAt_ = millis();
    lastSampleAt_ = startedAt_;
    lastMotionAt_ = startedAt_;
}

bool AirMouse::poll(int& dx, int& dy) {
    dx = 0;
    dy = 0;
    if (!active_ || !M5.Imu.update()) {
        return false;
    }

    float gx = 0;
    float gy = 0;
    float gz = 0;
    if (!M5.Imu.getGyro(&gx, &gy, &gz)) {
        return false;
    }

    const uint32_t now = millis();
    if (calibrating_) {
        biasX_ += gx;
        biasZ_ += gz;
        ++calibrationSamples_;
        if (now - startedAt_ >= kCalibrationMs && calibrationSamples_ > 0) {
            biasX_ /= calibrationSamples_;
            biasZ_ /= calibrationSamples_;
            calibrating_ = false;
            lastSampleAt_ = now;
        }
        return false;
    }

    const float dt = (now - lastSampleAt_) / 1000.0f;
    lastSampleAt_ = now;
    if (dt <= 0 || dt > 0.2f) {
        return false;  // first sample after a pause, skip to avoid a jump
    }

    const float yawRate = gz - biasZ_;
    const float pitchRate = gx - biasX_;
    if (fabsf(yawRate) > kDeadZoneDps || fabsf(pitchRate) > kDeadZoneDps) {
        lastMotionAt_ = now;
    }

    const float gain = kGain[constrain(speed_, 1, 5) - 1] * 10.0f;
    accX_ += -shape(yawRate, kDeadZoneDps) * gain * dt;
    accY_ += (invertY_ ? 1.0f : -1.0f) * shape(pitchRate, kDeadZoneDps) * gain * dt;

    dx = static_cast<int>(accX_);
    dy = static_cast<int>(accY_);
    accX_ -= dx;
    accY_ -= dy;
    return dx != 0 || dy != 0;
}

uint32_t AirMouse::idleMs() const {
    return millis() - lastMotionAt_;
}

}  // namespace lgremote
