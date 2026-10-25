// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#include "Settings.h"

#include <Preferences.h>

namespace lgremote {

namespace {

constexpr const char* kNamespace = "lgremote";

// Link modes are stored by name in JSON (readable, independent of the enum order).
constexpr const char* kModeKeys[] = {"wifi", "ir", "lan", "bluetooth", "usb"};

const char* modeKey(LinkMode mode) {
    const auto index = static_cast<size_t>(mode);
    return index < sizeof(kModeKeys) / sizeof(kModeKeys[0]) ? kModeKeys[index] : kModeKeys[0];
}

LinkMode modeFromKey(const char* key, LinkMode fallback) {
    for (size_t i = 0; i < sizeof(kModeKeys) / sizeof(kModeKeys[0]); ++i) {
        if (strcmp(key, kModeKeys[i]) == 0) {
            return static_cast<LinkMode>(i);
        }
    }
    return fallback;
}

}  // namespace

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

bool Settings::load() {
    Preferences prefs;
    if (!prefs.begin(kNamespace, true)) {
        return false;  // first boot or re-install: the namespace does not exist yet
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
    bleSetupDone = prefs.getBool("setup_ble", bleSetupDone);
    usbSetupDone = prefs.getBool("setup_usb", usbSetupDone);
    pointerSpeed = constrain(prefs.getUChar("ptr_speed", pointerSpeed), 1, 5);
    pointerInvertY = prefs.getBool("ptr_inv_y", pointerInvertY);
    brightness = prefs.getUChar("bright", brightness);
    dimTimeoutSec = prefs.getUShort("dim_s", dimTimeoutSec);
    offTimeoutSec = prefs.getUShort("off_s", offTimeoutSec);
    keyClick = prefs.getBool("click", keyClick);
    statusLed = prefs.getBool("led", statusLed);
    sdBackup = prefs.getBool("sd_backup", sdBackup);
    prefs.end();
    return true;
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
    prefs.putBool("setup_ble", bleSetupDone);
    prefs.putBool("setup_usb", usbSetupDone);
    prefs.putUChar("ptr_speed", pointerSpeed);
    prefs.putBool("ptr_inv_y", pointerInvertY);
    prefs.putUChar("bright", brightness);
    prefs.putUShort("dim_s", dimTimeoutSec);
    prefs.putUShort("off_s", offTimeoutSec);
    prefs.putBool("click", keyClick);
    prefs.putBool("led", statusLed);
    prefs.putBool("sd_backup", sdBackup);
    prefs.end();
}

void Settings::toJson(JsonObject root) const {
    root["schemaVersion"] = kSchemaVersion;
    root["app"] = "LGTVRemoteCardputer";
    root["linkMode"] = modeKey(linkMode);

    JsonObject wifi = root["wifi"].to<JsonObject>();
    wifi["ssid"] = wifiSsid;
    wifi["password"] = wifiPassword;

    JsonObject tv = root["tv"].to<JsonObject>();
    tv["host"] = tvHost;
    tv["name"] = tvName;
    tv["mac"] = tvMac;
    tv["clientKey"] = tvClientKey;
    tv["ipControlKeycode"] = tvKeycode;
    tv["pairing"] = tvUsePin ? "pin" : "prompt";

    JsonObject setup = root["setupDone"].to<JsonObject>();
    setup["bluetooth"] = bleSetupDone;
    setup["usb"] = usbSetupDone;

    JsonObject pointer = root["pointer"].to<JsonObject>();
    pointer["speed"] = pointerSpeed;
    pointer["invertY"] = pointerInvertY;

    JsonObject display = root["display"].to<JsonObject>();
    display["brightness"] = brightness;
    display["dimAfterSeconds"] = dimTimeoutSec;
    display["offAfterSeconds"] = offTimeoutSec;

    root["keyClick"] = keyClick;
    root["statusLight"] = statusLed;
    root["sdBackup"] = sdBackup;
}

bool Settings::fromJson(JsonObjectConst root) {
    const int version = root["schemaVersion"] | 0;
    if (version < 1 || version > kSchemaVersion) {
        return false;  // unknown or newer format: leave the current settings untouched
    }
    linkMode = modeFromKey(root["linkMode"] | "", linkMode);

    JsonObjectConst wifi = root["wifi"];
    wifiSsid = wifi["ssid"] | wifiSsid.c_str();
    wifiPassword = wifi["password"] | wifiPassword.c_str();

    JsonObjectConst tv = root["tv"];
    tvHost = tv["host"] | tvHost.c_str();
    tvName = tv["name"] | tvName.c_str();
    tvMac = tv["mac"] | tvMac.c_str();
    tvClientKey = tv["clientKey"] | tvClientKey.c_str();
    tvKeycode = tv["ipControlKeycode"] | tvKeycode.c_str();
    tvUsePin = strcmp(tv["pairing"] | (tvUsePin ? "pin" : "prompt"), "pin") == 0;

    JsonObjectConst setup = root["setupDone"];
    bleSetupDone = setup["bluetooth"] | bleSetupDone;
    usbSetupDone = setup["usb"] | usbSetupDone;

    JsonObjectConst pointer = root["pointer"];
    pointerSpeed = constrain(pointer["speed"] | static_cast<int>(pointerSpeed), 1, 5);
    pointerInvertY = pointer["invertY"] | pointerInvertY;

    JsonObjectConst display = root["display"];
    brightness = constrain(display["brightness"] | static_cast<int>(brightness), 0, 255);
    dimTimeoutSec = display["dimAfterSeconds"] | dimTimeoutSec;
    offTimeoutSec = display["offAfterSeconds"] | offTimeoutSec;

    keyClick = root["keyClick"] | keyClick;
    statusLed = root["statusLight"] | statusLed;
    sdBackup = root["sdBackup"] | sdBackup;
    return true;
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
        case LinkMode::Ir: return true;
        case LinkMode::Ble: return bleSetupDone;
        case LinkMode::Usb: return usbSetupDone;
        case LinkMode::Auto:
        case LinkMode::Lan: return !tvHost.isEmpty() && (!tvClientKey.isEmpty() || !tvKeycode.isEmpty());
        default: return false;
    }
}

}  // namespace lgremote
