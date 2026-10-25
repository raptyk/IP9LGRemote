// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

#include "app/Settings.h"
#include "ble/BleHidTransport.h"
#include "core/CommandRouter.h"
#include "hw/AirMouse.h"
#include "ipcontrol/IpControlClient.h"
#include "ir/IrTransport.h"
#include "net/WifiService.h"
#include "storage/SettingsBackup.h"
#include "ui/Screen.h"
#include "usb/UsbHidTransport.h"
#include "webos/TvDiscovery.h"
#include "webos/WebOsClient.h"

namespace lgremote {

// Owns all services and drives the main loop on top of the shared IP9Kit shell
// (screen stack, status bar, toasts, dimming, status LED).
class App : public ip9::Shell {
public:
    App();

    void begin();

    // Feedback for a sent key (shown on the remote screen).
    void keyClick();
    // Status LED feedback: a flash in the mode colour when a command left, red when it could not.
    void signalCommand(bool delivered);
    void setPointerActive(bool active) { pointerActive_ = active; }
    // Plays red, green, blue, white on the status LED (Settings > Test status light).
    void testStatusLed();

    Settings& settings() { return settings_; }
    // Saves to NVS and, when enabled and a card is inserted, to the SD card backup.
    void saveSettings();
    SettingsBackup& backup() { return backup_; }
    void applyWifiSettings();
    void applyTvSettings();
    void applyDisplaySettings();
    // Makes `mode` the active link and remembers it.
    void selectMode(LinkMode mode);

    CommandRouter& router() { return router_; }
    IrTransport& ir() { return ir_; }
    WebOsClient& webos() { return webos_; }
    IpControlClient& ipControl() { return ipControl_; }
    BleHidTransport& ble() { return ble_; }
    UsbHidTransport& usb() { return usb_; }
    WifiService& wifi() { return wifi_; }
    TvDiscovery& discovery() { return discovery_; }
    AirMouse& airMouse() { return airMouse_; }

protected:
    ip9::StatusLine statusLine(const ip9::Screen& top) override;
    // Wi-Fi, webOS, Bluetooth: pairing keys, learned MAC, completed setups.
    void onLoop() override;
    void updateStatusLed() override;
    bool keepAwake() const override;

private:
    // Loads NVS; returns true when the settings were restored from the SD card instead.
    bool loadSettings();
    LinkMode displayedMode() const;

    Settings settings_;
    SettingsBackup backup_;
    IrTransport ir_;
    WebOsClient webos_;
    IpControlClient ipControl_;
    BleHidTransport ble_;
    UsbHidTransport usb_;
    WifiService wifi_;
    TvDiscovery discovery_;
    AirMouse airMouse_;
    CommandRouter router_;

    bool textFieldHintShown_ = false;

    // Status LED state tracking.
    bool pointerActive_ = false;
    LinkMode ledMode_ = LinkMode::Count;
    bool ledLinkReady_ = false;
};

}  // namespace lgremote
