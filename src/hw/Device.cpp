// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#include "Device.h"

#include <M5Cardputer.h>

namespace lgremote {

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
    M5.Display.setBrightness(level);
}

}  // namespace lgremote
