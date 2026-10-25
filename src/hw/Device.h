// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

#include <stdint.h>

namespace lgremote {

enum class DeviceModel : uint8_t { Cardputer, CardputerAdv, Unknown };

namespace pins {
constexpr uint8_t kIrTx = 44;  // same on Cardputer and Cardputer ADV
}

// Board-level helpers: model detection, battery, backlight.
class Device {
public:
    void begin();

    DeviceModel model() const { return model_; }
    const char* modelName() const;
    bool hasImu() const { return hasImu_; }

    int batteryPercent() const;
    bool isCharging() const;

    void setBrightness(uint8_t level);
    uint8_t brightness() const { return brightness_; }

    // Cardputer ADV: the RGB LED is powered from the backlight line (GPIO38), so while the
    // status light is lit the backlight must not be dimmed below this level (0 = no limit).
    void setLedPowerFloor(uint8_t floor);

private:
    void applyBrightness();

    DeviceModel model_ = DeviceModel::Unknown;
    bool hasImu_ = false;
    uint8_t brightness_ = 0;
    uint8_t ledPowerFloor_ = 0;
};

}  // namespace lgremote
