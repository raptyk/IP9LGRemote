// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

#include <stdint.h>

namespace lgremote {

// Gyroscope based pointer (Cardputer ADV, BMI270).
// The device is held like a TV remote: the top edge (IR LED) points at the TV.
// Yaw (rotation around the Z axis) moves the cursor horizontally,
// pitch (rotation around the X axis) moves it vertically.
class AirMouse {
public:
    void begin(bool available);
    bool available() const { return available_; }

    // Starts a new session; the device should be held still for ~0.3 s to measure gyro bias.
    void start();
    void stop() { active_ = false; }
    bool active() const { return active_; }

    // Speed 1..5.
    void setSpeed(uint8_t speed) { speed_ = speed; }
    void setInvertY(bool invert) { invertY_ = invert; }

    // Reads the gyroscope; returns true when there is a non-zero movement to send.
    bool poll(int& dx, int& dy);

    // Milliseconds since the last real movement (used for auto-exit).
    uint32_t idleMs() const;

private:
    static constexpr uint32_t kCalibrationMs = 300;
    static constexpr float kDeadZoneDps = 1.8f;

    bool available_ = false;
    bool active_ = false;
    bool calibrating_ = false;
    uint32_t startedAt_ = 0;
    uint32_t lastSampleAt_ = 0;
    uint32_t lastMotionAt_ = 0;
    uint16_t calibrationSamples_ = 0;
    float biasX_ = 0;
    float biasZ_ = 0;
    float accX_ = 0;
    float accY_ = 0;
    uint8_t speed_ = 3;
    bool invertY_ = false;
};

}  // namespace lgremote
