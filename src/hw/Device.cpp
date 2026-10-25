// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#include "Device.h"

#include <M5Cardputer.h>

namespace lgremote {

namespace {

// On the Cardputer ADV GPIO38 drives both the display backlight (PWM) and the load switch
// of the RGB LED supply. M5GFX runs that PWM at only 256 Hz, which cuts the LED supply for
// ~1.4 ms every 4 ms so the LED resets and stays dark. At 20 kHz the gaps are microseconds.
constexpr uint32_t kAdvBacklightPwmHz = 20000;

}  // namespace

void Device::begin() {
    switch (M5.getBoard()) {
        case m5::board_t::board_M5Cardputer: model_ = DeviceModel::Cardputer; break;
        case m5::board_t::board_M5CardputerADV: model_ = DeviceModel::CardputerAdv; break;
        default: model_ = DeviceModel::Unknown; break;
    }
    hasImu_ = M5.Imu.isEnabled();

    if (model_ == DeviceModel::CardputerAdv) {
        auto* light = static_cast<lgfx::Light_PWM*>(M5.Display.light());
        if (light != nullptr) {
            auto config = light->config();
            config.freq = kAdvBacklightPwmHz;
            light->config(config);
            light->init(brightness_ == 0 ? 255 : brightness_);
        }
    }
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

void Device::setLedPowerFloor(uint8_t floor) {
    if (model_ != DeviceModel::CardputerAdv || floor == ledPowerFloor_) {
        return;  // only the ADV powers its LED from the backlight line
    }
    ledPowerFloor_ = floor;
    applyBrightness();
}

void Device::applyBrightness() {
    // A switched-off screen stays off; otherwise the backlight duty never drops below what the
    // LED supply needs while the status light is showing something.
    const uint8_t level = brightness_ == 0 ? 0 : max(brightness_, ledPowerFloor_);
    M5.Display.setBrightness(level);
}

}  // namespace lgremote
