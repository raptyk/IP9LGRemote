// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#include "Device.h"

#include <M5Cardputer.h>

namespace lgremote {

namespace {
// M5GFX maps 255 to a 100 % PWM duty (offset 16, 9-bit LEDC: duty 512/512), i.e. GPIO38
// stays high and the LED supply is never interrupted.
constexpr uint8_t kFullBacklight = 255;
}  // namespace

void Device::begin() {
    switch (M5.getBoard()) {
        case m5::board_t::board_M5Cardputer: model_ = DeviceModel::Cardputer; break;
        case m5::board_t::board_M5CardputerADV: model_ = DeviceModel::CardputerAdv; break;
        default: model_ = DeviceModel::Unknown; break;
    }
    hasImu_ = M5.Imu.isEnabled();
}

const char* Device::modelName() const {
    switch (model_) {
        case DeviceModel::Cardputer: return "Cardputer";
        case DeviceModel::CardputerAdv: return "Cardputer ADV";
        default: return "Unknown";
    }
}

int Device::batteryPercent() const {
    return M5.Power.getBatteryLevel();
}

bool Device::isCharging() const {
    return M5.Power.isCharging() == m5::Power_Class::is_charging_t::is_charging;
}

void Device::setBrightness(uint8_t level) {
    brightness_ = level;
    applyBrightness();
}

void Device::setLedPowerHold(bool hold) {
    hold = hold && model_ == DeviceModel::CardputerAdv;
    if (hold == ledPowerHold_) {
        return;
    }
    ledPowerHold_ = hold;
    applyBrightness();
}

void Device::applyBrightness() {
    // A switched-off screen stays off (the LED is unpowered then, which is fine).
    const uint8_t level = brightness_ == 0 ? 0 : (ledPowerHold_ ? kFullBacklight : brightness_);
    M5.Display.setBrightness(level);
}

}  // namespace lgremote
