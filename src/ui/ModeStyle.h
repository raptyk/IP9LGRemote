// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

#include "app/Settings.h"
#include "ui/Icons.h"
#include "ui/Strings.h"
#include "ui/Theme.h"

namespace lgremote::ui {

// Colour, icon and title of a link mode - used consistently on the home screen,
// the status bar chip and the remote screen so the active mode is always obvious.
inline uint16_t modeColour(LinkMode mode) {
    switch (mode) {
        case LinkMode::Ir: return theme::kModeIr;
        case LinkMode::Ble: return theme::kModeBluetooth;
        case LinkMode::Usb: return theme::kModeUsb;
        default: return theme::kModeWifi;
    }
}

inline icons::Icon modeIcon(LinkMode mode) {
    switch (mode) {
        case LinkMode::Ir: return icons::Icon::Infrared;
        case LinkMode::Ble: return icons::Icon::Bluetooth;
        case LinkMode::Usb: return icons::Icon::Usb;
        default: return icons::Icon::Wifi;
    }
}

inline const char* modeTitle(LinkMode mode) {
    switch (mode) {
        case LinkMode::Ir: return str::kCardIrTitle;
        case LinkMode::Ble: return str::kCardBleTitle;
        case LinkMode::Usb: return str::kCardUsbTitle;
        default: return str::kCardWifiTitle;
    }
}

}  // namespace lgremote::ui
