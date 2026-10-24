// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

#include <Arduino.h>

namespace lgremote {

// Persisted as a number: only append new values.
// Auto is the "Wi-Fi remote": network first, infrared as per-key fallback.
enum class LinkMode : uint8_t { Auto = 0, Ir, Lan, Ble, Usb, Count };

const char* linkModeName(LinkMode mode);

// User configuration persisted in NVS.
struct Settings {
    LinkMode linkMode = LinkMode::Auto;

    String wifiSsid;
    String wifiPassword;

    String tvHost;       // IP address of the TV
    String tvName;       // friendly name from discovery
    String tvMac;        // "AA:BB:CC:DD:EE:FF", used for Wake-on-LAN
    String tvClientKey;  // SSAP pairing key issued by the TV
    String tvKeycode;    // LG IP Control keycode (8 characters, optional)
    bool tvUsePin = false;  // pair with a PIN shown on the TV instead of the "Allow" prompt

    // First-run setup completed per link (Wi-Fi setup is derived from the pairing key).
    bool irSetupDone = false;
    bool bleSetupDone = false;
    bool usbSetupDone = false;

    uint8_t pointerSpeed = 3;  // 1..5
    bool pointerInvertY = false;
    uint8_t brightness = 160;  // 0..255
    uint16_t dimTimeoutSec = 20;
    uint16_t offTimeoutSec = 60;
    bool keyClick = false;

    void load();
    void save() const;
    void forgetTv();
    bool isSetUp(LinkMode mode) const;
};

}  // namespace lgremote
