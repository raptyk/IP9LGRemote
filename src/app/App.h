// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

#include <M5GFX.h>

#include <memory>
#include <vector>

#include "app/Settings.h"
#include "ble/BleHidTransport.h"
#include "core/CommandRouter.h"
#include "hw/AirMouse.h"
#include "hw/Device.h"
#include "hw/KeyboardInput.h"
#include "hw/StatusLed.h"
#include "ipcontrol/IpControlClient.h"
#include "ir/IrTransport.h"
#include "net/WifiService.h"
#include "ui/Screen.h"
#include "usb/UsbHidTransport.h"
#include "webos/TvDiscovery.h"
#include "webos/WebOsClient.h"

namespace lgremote {

// Owns all services and the screen stack; drives the main loop.
class App {
public:
    App();

    void begin();
    void loop();

    // Screen stack.
    void push(std::unique_ptr<Screen> screen);
    void pop();
    void popToRoot();
    // Pops screens until `target` is on top (only the top screen if `target` is not on the stack).
    void popTo(Screen* target);
    void invalidate() { dirty_ = true; }

    // Short message shown above the footer for ~1.8 s.
    void toast(const String& message);

    // Feedback for a sent key (shown on the remote screen).
    void keyClick();
    // Status LED feedback: a flash in the mode colour when a command left, red when it could not.
    void signalCommand(bool delivered);
    void setPointerActive(bool active) { pointerActive_ = active; }
    // Plays red, green, blue, white on the status LED (Settings > Test status light).
    void testStatusLed();

    Settings& settings() { return settings_; }
    void saveSettings() { settings_.save(); }
    void applyWifiSettings();
    void applyTvSettings();
    void applyDisplaySettings();
    // Makes `mode` the active link and remembers it.
    void selectMode(LinkMode mode);

    Device& device() { return device_; }
    CommandRouter& router() { return router_; }
    IrTransport& ir() { return ir_; }
    WebOsClient& webos() { return webos_; }
    IpControlClient& ipControl() { return ipControl_; }
    BleHidTransport& ble() { return ble_; }
    UsbHidTransport& usb() { return usb_; }
    WifiService& wifi() { return wifi_; }
    TvDiscovery& discovery() { return discovery_; }
    AirMouse& airMouse() { return airMouse_; }
    StatusLed& statusLed() { return statusLed_; }
    KeyboardInput& keyboard() { return keyboard_; }

private:
    void handleInput();
    void syncServices();
    void updatePower();
    void updateStatusLed();
    LinkMode displayedMode() const;
    void render();
    void drawStatusBar();
    void drawFooter(const char* text);
    void drawToast();

    Settings settings_;
    Device device_;
    KeyboardInput keyboard_;
    IrTransport ir_;
    WebOsClient webos_;
    IpControlClient ipControl_;
    BleHidTransport ble_;
    UsbHidTransport usb_;
    WifiService wifi_;
    TvDiscovery discovery_;
    AirMouse airMouse_;
    StatusLed statusLed_;
    CommandRouter router_;

    M5Canvas canvas_;
    std::vector<std::unique_ptr<Screen>> screens_;
    std::vector<std::unique_ptr<Screen>> retired_;  // popped screens, freed after the current event
    bool dirty_ = true;
    uint32_t lastRenderAt_ = 0;
    uint32_t lastStatusRefreshAt_ = 0;

    String toastText_;
    uint32_t toastUntil_ = 0;

    bool textFieldHintShown_ = false;

    // Status LED state tracking.
    bool pointerActive_ = false;
    LinkMode ledMode_ = LinkMode::Count;
    bool ledLinkReady_ = false;
    uint32_t lastBatteryBlinkAt_ = 0;
    uint32_t ledTestUntil_ = 0;

    uint32_t lastInputAt_ = 0;
    bool screenDimmed_ = false;
    bool screenOff_ = false;
};

}  // namespace lgremote
