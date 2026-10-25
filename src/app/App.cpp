// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#include "App.h"

#include <M5Cardputer.h>

#include <algorithm>

#include "ui/ModeStyle.h"
#include "ui/Painter.h"
#include "ui/Strings.h"
#include "ui/Theme.h"
#include "ui/screens/HomeScreen.h"

namespace lgremote {

namespace {

constexpr uint32_t kToastDurationMs = 1800;
constexpr uint32_t kStatusRefreshMs = 500;
constexpr uint8_t kDimBrightness = 24;
constexpr uint32_t kMinFrameIntervalMs = 33;  // ~30 fps is plenty and leaves CPU time for input
constexpr int kBatteryWidth = 52;             // battery icon + "100%" on the right of the status bar
constexpr uint16_t kLedGreetingStepMs = 150;
constexpr uint16_t kLedTestStepMs = 400;

}  // namespace

App::App()
    : ir_(pins::kIrTx),
      ble_(str::kBleDeviceName),
      router_(ir_, webos_, ipControl_, ble_, usb_),
      canvas_(&M5.Display) {}

void App::begin() {
    auto config = M5.config();
    config.internal_imu = true;
    config.internal_mic = false;
    config.serial_baudrate = 0;  // UART0 shares GPIO43/44 with the speaker clock and the IR LED
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

    ir_.begin();
    airMouse_.begin(device_.hasImu());
    statusLed_.begin();
    wifi_.begin();
    webos_.begin();
    ipControl_.begin();
    applyWifiSettings();
    applyTvSettings();
    applyDisplaySettings();
    if (settings_.statusLed) {
        // Short greeting in the four mode colours: shows at a glance that the LED works.
        const uint32_t greeting[] = {ui::modeLedColour(LinkMode::Auto), ui::modeLedColour(LinkMode::Ir),
                                     ui::modeLedColour(LinkMode::Ble), ui::modeLedColour(LinkMode::Usb)};
        statusLed_.playSequence(greeting, 4, kLedGreetingStepMs);
    }
    // Bluetooth / USB start only when the user picks them on the home screen.
    const bool networkMode = settings_.linkMode == LinkMode::Auto || settings_.linkMode == LinkMode::Lan;
    router_.setMode(networkMode ? settings_.linkMode : LinkMode::Ir);

    log_i("%s %s on %s, IMU: %s", str::kAppName, APP_VERSION, device_.modelName(), device_.hasImu() ? "yes" : "no");

    push(std::make_unique<HomeScreen>(*this));
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
    updateStatusLed();
    statusLed_.loop();

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
        return;  // the home screen is the root and never closes
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

void App::popTo(Screen* target) {
    const bool onStack = std::any_of(screens_.begin(), screens_.end(),
                                     [target](const std::unique_ptr<Screen>& screen) { return screen.get() == target; });
    if (!onStack) {
        pop();
        return;
    }
    while (screens_.size() > 1 && screens_.back().get() != target) {
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

void App::warn(const String& message) {
    toast(message);
    signalWarning();
}

void App::signalWarning() {
    // Once per event and short on purpose: a hint, not an alarm.
    statusLed_.blink(ui::kLedError, 2, 70, 90, true);
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
    webos_.configure(settings_.tvHost, settings_.tvClientKey,
                     settings_.tvUsePin ? PairingMethod::Pin : PairingMethod::Prompt);
    ipControl_.configure(settings_.tvHost, settings_.tvKeycode);
    router_.setWakeMac(settings_.tvMac);
}

void App::applyDisplaySettings() {
    // ADV: the LED is powered from the backlight rail, so an enabled status light keeps the
    // backlight at full duty (see Device::setLedPowerHold).
    device_.setLedPowerHold(settings_.statusLed);
    device_.setBrightness(settings_.brightness);
    airMouse_.setSpeed(settings_.pointerSpeed);
    airMouse_.setInvertY(settings_.pointerInvertY);
    statusLed_.setEnabled(settings_.statusLed);
}

void App::selectMode(LinkMode mode) {
    settings_.linkMode = mode;
    router_.setMode(mode);
    saveSettings();
    dirty_ = true;
}

void App::testStatusLed() {
    const uint32_t colours[] = {0xFF0000, 0x00FF00, 0x0000FF, 0xFFFFFF};
    statusLed_.playSequence(colours, 4, kLedTestStepMs);
}

void App::signalCommand(bool delivered) {
    if (delivered) {
        statusLed_.flash(ui::modeLedColour(displayedMode()));
    } else {
        statusLed_.blink(ui::kLedError, 2, 60, 80);
    }
}

LinkMode App::displayedMode() const {
    return !screens_.empty() && screens_.back()->forcesInfrared() ? LinkMode::Ir : settings_.linkMode;
}

void App::updateStatusLed() {
    // Signals (see docs/UX_DESIGN.md "Status light"):
    //   flash in the mode colour = command sent, two red blinks = command not delivered,
    //   slow pulse in the mode colour = connecting / waiting for the TV,
    //   two green blinks = connected, three red blinks = connection lost,
    //   steady glow = pointer mode, short red blink every 10 s = battery low.
    const LinkMode mode = displayedMode();
    bool ready = false;
    bool waiting = false;
    switch (mode) {
        case LinkMode::Ble:
            ready = ble_.isConnected();
            waiting = ble_.started() && !ready;
            break;
        case LinkMode::Usb:
            ready = usb_.isMounted();
            waiting = usb_.started() && !ready;
            break;
        case LinkMode::Auto:
        case LinkMode::Lan:
            ready = webos_.isReady() || ipControl_.isReady();
            waiting = !ready && (webos_.state() == LinkState::Connecting ||
                                 (!settings_.wifiSsid.isEmpty() && !wifi_.isConnected()));
            break;
        default:
            break;
    }

    if (mode == ledMode_ && ready != ledLinkReady_) {
        if (ready) {
            statusLed_.blink(ui::kLedSuccess, 2);
        } else {
            statusLed_.blink(ui::kLedError, 3);
        }
    }
    ledMode_ = mode;
    ledLinkReady_ = ready;

    const bool onLinkScreen = !screens_.empty() && screens_.back()->showsLinkStatus();
    const uint32_t colour = ui::modeLedColour(mode);
    if (pointerActive_) {
        statusLed_.setBackground(LedPattern::Steady, colour);
    } else if (waiting && onLinkScreen) {
        statusLed_.setBackground(LedPattern::Breathing, colour);
    } else {
        statusLed_.setBackground(LedPattern::Off, 0);
        if (millis() - lastBatteryBlinkAt_ > 10000) {
            lastBatteryBlinkAt_ = millis();  // the battery ADC is read at most every 10 s
            const int battery = device_.batteryPercent();
            if (battery >= 0 && battery <= 10 && !device_.isCharging()) {
                statusLed_.flash(ui::kLedError, 40);
            }
        }
    }
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

    // A first successful connection completes the Bluetooth / USB setup.
    if (!settings_.bleSetupDone && ble_.isConnected()) {
        settings_.bleSetupDone = true;
        saveSettings();
    }
    if (!settings_.usbSetupDone && usb_.isMounted()) {
        settings_.usbSetupDone = true;
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

    // Link mode chip: mode name on the mode's own colour (blue Wi-Fi, orange IR, ...).
    const Screen& screen = *screens_.back();
    const LinkMode shownMode = screen.forcesInfrared() ? LinkMode::Ir : settings_.linkMode;
    const char* mode = screen.showsLinkStatus() ? linkModeName(shownMode) : "LG";
    const uint16_t chipColour = screen.showsLinkStatus() ? ui::modeColour(shownMode) : kAccent;
    canvas_.setFont(&fonts::Font0);
    canvas_.setTextSize(1);
    const int chipWidth = canvas_.textWidth(mode) + 12;
    canvas_.fillRoundRect(2, 2, chipWidth, kStatusBarHeight - 4, 4, chipColour);
    canvas_.setTextColor(kText);
    canvas_.setTextDatum(middle_center);
    canvas_.drawString(mode, 2 + chipWidth / 2, kStatusBarHeight / 2);

    // Link state: coloured dot + what the remote talks to right now.
    uint16_t dotColour = kOk;
    String description;
    const String tvLabel = settings_.tvName.isEmpty() ? settings_.tvHost : settings_.tvName;
    if (!screen.showsLinkStatus()) {
        description = str::kHomeStatus;
        dotColour = kAccent;
    } else {
        switch (shownMode) {
            case LinkMode::Ir:
                description = str::kInfrared;
                break;
            case LinkMode::Ble:
                description = ble_.isConnected() ? str::kBleConnected : str::kBleWaiting;
                dotColour = ble_.isConnected() ? kOk : kWarn;
                break;
            case LinkMode::Usb:
                description = usb_.isMounted() ? str::kBleConnected : str::kUsbNotConnected;
                dotColour = usb_.isMounted() ? kOk : kWarn;
                break;
            case LinkMode::Auto:
            case LinkMode::Lan:
            default:
                if (webos_.isReady() || ipControl_.isReady()) {
                    description = tvLabel;
                } else {
                    description = webOsStatusText(webos_.status());
                    if (settings_.linkMode == LinkMode::Auto) {
                        description += str::kViaIrSuffix;
                    }
                    dotColour = webos_.state() == LinkState::Connecting ? kWarn : kError;
                }
                break;
        }
    }

    const int dotX = 2 + chipWidth + 8;
    canvas_.fillCircle(dotX, kStatusBarHeight / 2, 3, dotColour);
    canvas_.setTextDatum(middle_left);
    canvas_.setTextColor(kText);
    painter::drawClippedText(canvas_, description, dotX + 7, kStatusBarHeight / 2,
                             kScreenWidth - kBatteryWidth - (dotX + 7));

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
    canvas_.setFont(&fonts::Font2);
    canvas_.setTextSize(1);
    const int maxTextWidth = kScreenWidth - 24;
    std::vector<String> lines = painter::wrapText(canvas_, toastText_, maxTextWidth);
    if (lines.size() > 2) {
        lines.resize(2);
    }
    int textWidth = 0;
    for (const String& line : lines) {
        textWidth = max<int>(textWidth, canvas_.textWidth(line));
    }
    const int lineHeight = 15;
    const int width = min<int>(textWidth + 16, kScreenWidth - 8);
    const int height = static_cast<int>(lines.size()) * lineHeight + 8;
    const int x = (kScreenWidth - width) / 2;
    const int y = kScreenHeight - kFooterHeight - height - 4;
    canvas_.fillRoundRect(x, y, width, height, 6, kSurfaceHigh);
    canvas_.drawRoundRect(x, y, width, height, 6, kAccent);
    canvas_.setTextColor(kText);
    canvas_.setTextDatum(top_center);
    for (size_t i = 0; i < lines.size(); ++i) {
        painter::drawClippedText(canvas_, lines[i], kScreenWidth / 2, y + 4 + i * lineHeight, maxTextWidth);
    }
}

}  // namespace lgremote
