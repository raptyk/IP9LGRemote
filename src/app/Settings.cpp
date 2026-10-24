// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#include "Settings.h"

#include <Preferences.h>

namespace lgremote {

namespace {
constexpr const char* kNamespace = "lgremote";
}

const char* linkModeName(LinkMode mode) {
    switch (mode) {
        case LinkMode::Auto: return "WI-FI";
        case LinkMode::Ir: return "IR";
        case LinkMode::Lan: return "LAN";
        case LinkMode::Ble: return "BT";
        case LinkMode::Usb: return "USB";
        default: return "?";
    }
}

void Settings::load() {
    Preferences prefs;
    if (!prefs.begin(kNamespace, true)) {
        return;  // first boot, namespace does not exist yet
    }
    uint8_t mode = prefs.getUChar("link", static_cast<uint8_t>(LinkMode::Auto));
    linkMode = mode < static_cast<uint8_t>(LinkMode::Count) ? static_cast<LinkMode>(mode) : LinkMode::Auto;
    wifiSsid = prefs.getString("wifi_ssid", "");
    wifiPassword = prefs.getString("wifi_pass", "");
    tvHost = prefs.getString("tv_host", "");
    tvName = prefs.getString("tv_name", "");
    tvMac = prefs.getString("tv_mac", "");
    tvClientKey = prefs.getString("tv_key", "");
    tvKeycode = prefs.getString("tv_ipc", "");
    tvUsePin = prefs.getBool("tv_pin", tvUsePin);
    irSetupDone = prefs.getBool("setup_ir", irSetupDone);
    bleSetupDone = prefs.getBool("setup_ble", bleSetupDone);
    usbSetupDone = prefs.getBool("setup_usb", usbSetupDone);
    pointerSpeed = constrain(prefs.getUChar("ptr_speed", pointerSpeed), 1, 5);
    pointerInvertY = prefs.getBool("ptr_inv_y", pointerInvertY);
    brightness = prefs.getUChar("bright", brightness);
    dimTimeoutSec = prefs.getUShort("dim_s", dimTimeoutSec);
    offTimeoutSec = prefs.getUShort("off_s", offTimeoutSec);
    keyClick = prefs.getBool("click", keyClick);
    prefs.end();
}

void Settings::save() const {
    Preferences prefs;
    if (!prefs.begin(kNamespace, false)) {
        log_e("Cannot open NVS namespace %s", kNamespace);
        return;
    }
    prefs.putUChar("link", static_cast<uint8_t>(linkMode));
    prefs.putString("wifi_ssid", wifiSsid);
    prefs.putString("wifi_pass", wifiPassword);
    prefs.putString("tv_host", tvHost);
    prefs.putString("tv_name", tvName);
    prefs.putString("tv_mac", tvMac);
    prefs.putString("tv_key", tvClientKey);
    prefs.putString("tv_ipc", tvKeycode);
    prefs.putBool("tv_pin", tvUsePin);
    prefs.putBool("setup_ir", irSetupDone);
    prefs.putBool("setup_ble", bleSetupDone);
    prefs.putBool("setup_usb", usbSetupDone);
    prefs.putUChar("ptr_speed", pointerSpeed);
    prefs.putBool("ptr_inv_y", pointerInvertY);
    prefs.putUChar("bright", brightness);
    prefs.putUShort("dim_s", dimTimeoutSec);
    prefs.putUShort("off_s", offTimeoutSec);
    prefs.putBool("click", keyClick);
    prefs.end();
}

void Settings::forgetTv() {
    tvHost = "";
    tvName = "";
    tvMac = "";
    tvClientKey = "";
    tvKeycode = "";
    tvUsePin = false;
}

bool Settings::isSetUp(LinkMode mode) const {
    switch (mode) {
        case LinkMode::Ir: return irSetupDone;
        case LinkMode::Ble: return bleSetupDone;
        case LinkMode::Usb: return usbSetupDone;
        case LinkMode::Auto:
        case LinkMode::Lan: return !tvHost.isEmpty() && (!tvClientKey.isEmpty() || !tvKeycode.isEmpty());
        default: return false;
    }
}

}  // namespace lgremote
