// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#include "App.h"

#include <M5Cardputer.h>

#include "ui/Painter.h"
#include "ui/Strings.h"
#include "ui/Theme.h"
#include "ui/screens/RemoteScreen.h"

namespace lgremote {

namespace {

constexpr uint32_t kToastDurationMs = 1800;
constexpr uint32_t kStatusRefreshMs = 500;
constexpr uint8_t kDimBrightness = 24;
constexpr uint32_t kMinFrameIntervalMs = 33;  // ~30 fps is plenty and leaves CPU time for input

}  // namespace

App::App()
    : ir_(pins::kIrTx),
      ble_(str::kBleDeviceName),
      router_(ir_, webos_, ipControl_, ble_),
      canvas_(&M5.Display) {}

void App::begin() {
    auto config = M5.config();
    config.internal_imu = true;
    config.internal_mic = false;
    M5Cardputer.begin(config, true);
    M5.Display.setRotation(1);

    device_.begin();
    settings_.load();

    canvas_.setColorDepth(16);
    if (!canvas_.createSprite(theme::kScreenWidth, theme::kScreenHeight)) {
        // Not enough contiguous RAM for 16-bit: fall back to 8-bit colour.
        canvas_.setColorDepth(8);
        canvas_.createSprite(theme::kScreenWidth, theme::kScreenHeight);
    }

    applyDisplaySettings();
    ir_.begin();
    airMouse_.begin(device_.hasImu());
    wifi_.begin();
    webos_.begin();
    ipControl_.begin();
    applyWifiSettings();
    applyTvSettings();
    router_.setMode(settings_.linkMode);

    log_i("%s %s on %s, IMU: %s", str::kAppName, APP_VERSION, device_.modelName(), device_.hasImu() ? "yes" : "no");

    push(std::make_unique<RemoteScreen>(*this));
    lastInputAt_ = millis();
}

void App::loop() {
    M5Cardputer.update();
    keyboard_.update();
    handleInput();
    syncServices();
    if (!screens_.empty()) {
        screens_.back()->tick();
    }
    retired_.clear();
    updatePower();

    const uint32_t now = millis();
    if (now - lastStatusRefreshAt_ >= kStatusRefreshMs) {
        lastStatusRefreshAt_ = now;
        dirty_ = true;  // status bar (battery, link state) may have changed
    }
    if (toastUntil_ != 0 && now > toastUntil_) {
        toastUntil_ = 0;
        dirty_ = true;
    }
    if (dirty_ && !screenOff_ && now - lastRenderAt_ >= kMinFrameIntervalMs) {
        render();
    }
}

void App::push(std::unique_ptr<Screen> screen) {
    screens_.push_back(std::move(screen));
    screens_.back()->onEnter();
    dirty_ = true;
}

void App::pop() {
    if (screens_.size() <= 1) {
        return;  // the remote screen is the root and never closes
    }
    retired_.push_back(std::move(screens_.back()));
    screens_.pop_back();
    screens_.back()->onEnter();
    dirty_ = true;
}

void App::popToRoot() {
    while (screens_.size() > 1) {
        retired_.push_back(std::move(screens_.back()));
        screens_.pop_back();
    }
    screens_.back()->onEnter();
    dirty_ = true;
}

void App::toast(const String& message) {
    toastText_ = message;
    toastUntil_ = millis() + kToastDurationMs;
    dirty_ = true;
}

void App::keyClick() {
    if (settings_.keyClick) {
        M5Cardputer.Speaker.tone(3200, 6);
    }
}

void App::applyWifiSettings() {
    if (settings_.wifiSsid.isEmpty()) {
        wifi_.disconnect();
    } else {
        wifi_.connect(settings_.wifiSsid, settings_.wifiPassword);
    }
}

void App::applyTvSettings() {
    webos_.configure(settings_.tvHost, settings_.tvClientKey);
    ipControl_.configure(settings_.tvHost, settings_.tvKeycode);
    router_.setWakeMac(settings_.tvMac);
}

void App::applyDisplaySettings() {
    device_.setBrightness(settings_.brightness);
    airMouse_.setSpeed(settings_.pointerSpeed);
    airMouse_.setInvertY(settings_.pointerInvertY);
}

void App::cycleLinkMode() {
    uint8_t next = (static_cast<uint8_t>(settings_.linkMode) + 1) % static_cast<uint8_t>(LinkMode::Count);
    settings_.linkMode = static_cast<LinkMode>(next);
    router_.setMode(settings_.linkMode);
    saveSettings();
    dirty_ = true;
}

void App::handleInput() {
    while (keyboard_.hasEvent()) {
        KeyEvent event = keyboard_.nextEvent();
        lastInputAt_ = millis();
        if (screenOff_) {
            // The key that wakes the screen is swallowed, so nothing is sent by accident.
            screenOff_ = false;
            screenDimmed_ = false;
            device_.setBrightness(settings_.brightness);
            keyboard_.suppressHeldKeys();
            dirty_ = true;
            continue;
        }
        if (screenDimmed_) {
            screenDimmed_ = false;
            device_.setBrightness(settings_.brightness);
        }
        if (!screens_.empty()) {
            screens_.back()->onKey(event);
        }
        dirty_ = true;
    }
}

void App::syncServices() {
    wifi_.loop();
    const bool online = wifi_.isConnected();
    webos_.setNetworkAvailable(online);
    ipControl_.setNetworkAvailable(online);
    ble_.loop();

    // Hint when the TV opens its on-screen keyboard: typing on the Cardputer is faster.
    const bool textField = webos_.isReady() && webos_.textFieldFocused();
    if (textField && !textFieldHintShown_) {
        toast(str::kTypeHint);
    }
    textFieldHintShown_ = textField;

    String value;
    if (webos_.takeNewClientKey(value)) {
        settings_.tvClientKey = value;
        saveSettings();
    }
    if (webos_.takeLearnedMac(value) && settings_.tvMac != value) {
        settings_.tvMac = value;
        router_.setWakeMac(value);
        saveSettings();
    }
}

void App::updatePower() {
    const uint32_t idle = millis() - lastInputAt_;
    if (keyboard_.anyHeld() || airMouse_.active()) {
        lastInputAt_ = millis();
        return;
    }
    if (!screenOff_ && settings_.offTimeoutSec > 0 && idle > settings_.offTimeoutSec * 1000UL) {
        screenOff_ = true;
        device_.setBrightness(0);
    } else if (!screenDimmed_ && !screenOff_ && settings_.dimTimeoutSec > 0 && idle > settings_.dimTimeoutSec * 1000UL) {
        screenDimmed_ = true;
        device_.setBrightness(min<uint8_t>(kDimBrightness, settings_.brightness));
    }
}

void App::render() {
    dirty_ = false;
    lastRenderAt_ = millis();
    canvas_.fillScreen(theme::kBackground);

    const int contentTop = theme::kStatusBarHeight;
    const int contentHeight = theme::kScreenHeight - theme::kStatusBarHeight - theme::kFooterHeight;
    Screen& screen = *screens_.back();
    screen.draw(canvas_, contentTop, contentHeight);

    drawStatusBar();
    drawFooter(screen.footer());
    drawToast();
    canvas_.pushSprite(0, 0);
}

void App::drawStatusBar() {
    using namespace theme;
    canvas_.fillRect(0, 0, kScreenWidth, kStatusBarHeight, kSurface);

    // Link mode chip.
    const char* mode = linkModeName(settings_.linkMode);
    canvas_.setFont(&fonts::Font0);
    canvas_.setTextSize(1);
    const int chipWidth = canvas_.textWidth(mode) + 10;
    canvas_.fillRoundRect(3, 3, chipWidth, kStatusBarHeight - 6, 3, kAccent);
    canvas_.setTextColor(kText);
    canvas_.setTextDatum(middle_center);
    canvas_.drawString(mode, 3 + chipWidth / 2, kStatusBarHeight / 2);

    // Link state: coloured dot + description.
    Transport& link = router_.primary();
    uint16_t dotColour = kOk;
    String description = link.name();
    if (&link == static_cast<Transport*>(&webos_)) {
        if (webos_.isReady()) {
            description += "  ";
            description += settings_.tvName.isEmpty() ? settings_.tvHost : settings_.tvName;
        } else {
            description += "  ";
            description += webOsStatusText(webos_.status());
        }
    } else if (&link == static_cast<Transport*>(&ipControl_)) {
        description += "  ";
        description += settings_.tvName.isEmpty() ? settings_.tvHost : settings_.tvName;
    } else if (&link == static_cast<Transport*>(&ble_)) {
        description += "  ";
        description += ble_.isConnected() ? str::kBleConnected : str::kBleWaiting;
    } else if (settings_.linkMode == LinkMode::Auto && !settings_.tvHost.isEmpty()) {
        description += "  ";
        description += webOsStatusText(webos_.status());
    }
    switch (link.state()) {
        case LinkState::Ready: dotColour = kOk; break;
        case LinkState::Connecting: dotColour = kWarn; break;
        case LinkState::Off:
        case LinkState::Error: dotColour = kError; break;
    }
    if (settings_.linkMode == LinkMode::Auto && &link == static_cast<Transport*>(&ir_)) {
        dotColour = settings_.tvHost.isEmpty() ? kOk : kWarn;  // IR works, network fallback pending
    }

    const int dotX = 3 + chipWidth + 8;
    canvas_.fillCircle(dotX, kStatusBarHeight / 2, 3, dotColour);
    canvas_.setTextDatum(middle_left);
    canvas_.setTextColor(kText);
    painter::drawClippedText(canvas_, description, dotX + 7, kStatusBarHeight / 2, 150 - chipWidth);

    // Battery.
    const int battery = device_.batteryPercent();
    painter::drawBattery(canvas_, kScreenWidth - 4, kStatusBarHeight / 2, battery, device_.isCharging());
}

void App::drawFooter(const char* text) {
    using namespace theme;
    const int top = kScreenHeight - kFooterHeight;
    canvas_.fillRect(0, top, kScreenWidth, kFooterHeight, kSurface);
    if (text == nullptr) {
        return;
    }
    canvas_.setFont(&fonts::Font0);
    canvas_.setTextSize(1);
    canvas_.setTextColor(kTextMuted);
    canvas_.setTextDatum(middle_center);
    canvas_.drawString(text, kScreenWidth / 2, top + kFooterHeight / 2);
}

void App::drawToast() {
    using namespace theme;
    if (toastUntil_ == 0) {
        return;
    }
    canvas_.setFont(&fonts::FreeSans9pt7b);
    canvas_.setTextSize(1);
    const int width = min<int>(canvas_.textWidth(toastText_) + 16, kScreenWidth - 8);
    const int height = 22;
    const int x = (kScreenWidth - width) / 2;
    const int y = kScreenHeight - kFooterHeight - height - 4;
    canvas_.fillRoundRect(x, y, width, height, 6, kSurfaceHigh);
    canvas_.drawRoundRect(x, y, width, height, 6, kAccent);
    canvas_.setTextColor(kText);
    canvas_.setTextDatum(middle_center);
    canvas_.drawString(toastText_, kScreenWidth / 2, y + height / 2 + 1);
}

}  // namespace lgremote
