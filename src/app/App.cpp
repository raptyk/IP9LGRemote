// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#include "App.h"

#include <M5Cardputer.h>

#include "ui/ModeStyle.h"
#include "ui/Strings.h"
#include "ui/Theme.h"
#include "ui/screens/HomeScreen.h"

namespace lgremote {

namespace {

constexpr uint32_t kToastDurationMs = 1800;
constexpr uint16_t kLedGreetingStepMs = 150;
constexpr uint16_t kLedTestStepMs = 400;

}  // namespace

App::App() : ir_(pins::kIrTx), ble_(str::kBleDeviceName), router_(ir_, webos_, ipControl_, ble_, usb_) {}

void App::begin() {
    auto config = M5.config();
    config.internal_imu = true;
    config.internal_mic = false;
    config.serial_baudrate = 0;  // UART0 shares GPIO43/44 with the speaker clock and the IR LED
    beginShell(config, theme::kAccent);
    setToastDuration(kToastDurationMs);

    const bool restoredFromCard = loadSettings();

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
    if (restoredFromCard) {
        toast(str::kBackupRestored);
    }
}

bool App::loadSettings() {
    const bool inNvs = settings_.load();
    // After a re-install NVS can be empty: bring the pairing back from the SD card copy.
    if (!backup_.mount() || (inNvs && !settings_.isBlank())) {
        return false;
    }
    Settings fromCard = settings_;
    if (!backup_.load(fromCard) || fromCard.isBlank()) {
        return false;
    }
    settings_ = fromCard;
    settings_.save();
    return true;
}

void App::saveSettings() {
    settings_.save();
    if (settings_.sdBackup) {
        backup_.save(settings_);
    }
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
    setDisplayPower(settings_.brightness, settings_.dimTimeoutSec * 1000UL, settings_.offTimeoutSec * 1000UL,
                    settings_.statusLed);
    airMouse_.setSpeed(settings_.pointerSpeed);
    airMouse_.setInvertY(settings_.pointerInvertY);
}

void App::selectMode(LinkMode mode) {
    settings_.linkMode = mode;
    router_.setMode(mode);
    saveSettings();
    invalidate();
}

void App::testStatusLed() {
    const uint32_t colours[] = {0xFF0000, 0x00FF00, 0x0000FF, 0xFFFFFF};
    statusLed_.playSequence(colours, 4, kLedTestStepMs);
}

void App::signalCommand(bool delivered) {
    if (delivered) {
        statusLed_.flash(ui::modeLedColour(displayedMode()));
    } else {
        statusLed_.blink(led::kError, 2, 60, 80);
    }
}

LinkMode App::displayedMode() const {
    const ip9::Screen* screen = top();
    return screen != nullptr && (screen->traits() & trait::kInfraredOnly) ? LinkMode::Ir : settings_.linkMode;
}

bool App::keepAwake() const {
    return airMouse_.active();
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
            statusLed_.blink(led::kSuccess, 2);
        } else {
            statusLed_.blink(led::kError, 3);
        }
    }
    ledMode_ = mode;
    ledLinkReady_ = ready;

    // Screens with their own chip (home launcher) are not tied to a link.
    const bool onLinkScreen = top() != nullptr && top()->chipLabel() == nullptr;
    const uint32_t colour = ui::modeLedColour(mode);
    if (pointerActive_) {
        statusLed_.setBackground(LedPattern::Steady, colour);
    } else if (waiting && onLinkScreen) {
        statusLed_.setBackground(LedPattern::Breathing, colour);
    } else {
        statusLed_.setBackground(LedPattern::Off, 0);
        blinkIfBatteryLow();
    }
}

void App::onLoop() {
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

StatusLine App::statusLine(const ip9::Screen& top) {
    using namespace theme;
    if (top.chipLabel() != nullptr) {
        // Home launcher: app chip instead of the link state.
        return {top.chipLabel(), top.chipColour(), kAccent, str::kHomeStatus};
    }

    // Link mode chip: mode name on the mode's own colour (blue Wi-Fi, orange IR, ...),
    // then what the remote talks to right now.
    const LinkMode shownMode = displayedMode();
    StatusLine line{linkModeName(shownMode), ui::modeColour(shownMode), kOk, String()};
    const String tvLabel = settings_.tvName.isEmpty() ? settings_.tvHost : settings_.tvName;
    switch (shownMode) {
        case LinkMode::Ir:
            line.text = str::kInfrared;
            break;
        case LinkMode::Ble:
            line.text = ble_.isConnected() ? str::kBleConnected : str::kBleWaiting;
            line.dotColour = ble_.isConnected() ? kOk : kWarn;
            break;
        case LinkMode::Usb:
            line.text = usb_.isMounted() ? str::kBleConnected : str::kUsbNotConnected;
            line.dotColour = usb_.isMounted() ? kOk : kWarn;
            break;
        case LinkMode::Auto:
        case LinkMode::Lan:
        default:
            if (webos_.isReady() || ipControl_.isReady()) {
                line.text = tvLabel;
            } else {
                line.text = webOsStatusText(webos_.status());
                if (settings_.linkMode == LinkMode::Auto) {
                    line.text += str::kViaIrSuffix;
                }
                line.dotColour = webos_.state() == LinkState::Connecting ? kWarn : kError;
            }
            break;
    }
    return line;
}

}  // namespace lgremote
