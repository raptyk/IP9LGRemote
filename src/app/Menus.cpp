// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#include "Menus.h"

#include "app/App.h"
#include "ble/HidReportMap.h"
#include "ui/Strings.h"
#include "ui/screens/ListScreen.h"
#include "ui/screens/MessageScreens.h"
#include "ui/screens/RemoteScreen.h"
#include "ui/screens/TextFieldScreen.h"
#include "webos/WakeOnLan.h"

namespace lgremote {

namespace {

constexpr uint8_t kBrightnessLevels[] = {40, 80, 120, 160, 210, 255};
constexpr size_t kBrightnessLevelCount = sizeof(kBrightnessLevels) / sizeof(kBrightnessLevels[0]);

struct AppShortcut {
    const char* title;
    const char* webOsId;
    uint8_t irCommand;  // dedicated IR key on LG remotes, 0 if none
};

// App IDs from LG webOS (see docs/PROTOCOLS.md); IR codes from Flipper-IRDB.
constexpr AppShortcut kApps[] = {
    {"Netflix", "netflix", 0x56},
    {"YouTube", "youtube.leanback.v4", 0},
    {"Prime Video", "amazon", 0x5C},
    {"Disney+", "com.disney.disneyplus-prod", 0x31},
    {"Max", "com.hbo.hbomax", 0},
    {"Apple TV", "com.apple.appletv", 0},
    {"Spotify", "spotify-beehive", 0},
    {"Plex", "cdp-30", 0},
    {"Web browser", "com.webos.app.browser", 0},
    {"Live TV", "com.webos.app.livetv", 0},
    {"TV settings", "com.palm.app.settings", 0},
};

String onOff(bool value) {
    return value ? str::kOn : str::kOff;
}

size_t brightnessIndex(uint8_t brightness) {
    size_t best = 0;
    for (size_t i = 0; i < kBrightnessLevelCount; ++i) {
        if (abs(kBrightnessLevels[i] - brightness) < abs(kBrightnessLevels[best] - brightness)) {
            best = i;
        }
    }
    return best;
}

std::unique_ptr<Screen> makeTvMenu(App& app) {
    auto menu = std::make_unique<ListScreen>(app, str::kTvTitle);
    Screen* self = menu.get();
    std::vector<ListItem> items;
    items.push_back({str::kTvSearch, nullptr,
                     [&app, self]() {
                         if (!app.wifi().isConnected()) {
                             app.toast(str::kTvNeedWifi);
                             return;
                         }
                         app.push(std::make_unique<TvSearchScreen>(app, self));
                     },
                     nullptr, str::kTvSearchText});
    items.push_back({str::kTvManual,
                     [&app]() { return app.settings().tvHost.isEmpty() ? String(str::kNotSet) : app.settings().tvHost; },
                     [&app, self]() { promptTvAddress(app, self); }, nullptr, str::kTvManualText});
    items.push_back({str::kTvPairingMethod,
                     [&app]() { return String(app.settings().tvUsePin ? str::kTvMethodPin : str::kTvMethodPrompt); },
                     nullptr,
                     [&app](int) {
                         app.settings().tvUsePin = !app.settings().tvUsePin;
                         app.saveSettings();
                     },
                     str::kTvPairingMethodText});
    items.push_back({str::kTvPairAgain, nullptr,
                     [&app, self]() {
                         if (app.settings().tvHost.isEmpty()) {
                             app.toast(str::kSetUpTvFirst);
                             return;
                         }
                         app.settings().tvClientKey = "";
                         app.saveSettings();
                         app.applyTvSettings();
                         app.push(std::make_unique<TvPairingScreen>(app, self));
                     },
                     nullptr, str::kTvPairAgainText});
    items.push_back({str::kTvKeycode,
                     [&app]() { return app.settings().tvKeycode.isEmpty() ? String(str::kNotSet) : String("********"); },
                     [&app]() {
                         app.push(std::make_unique<TextFieldScreen>(
                             app, str::kTvKeycodePrompt, app.settings().tvKeycode,
                             [&app](const String& text) {
                                 String keycode = text;
                                 keycode.trim();
                                 keycode.toUpperCase();
                                 if (!keycode.isEmpty() && !IpControlClient::isValidKeycode(keycode)) {
                                     app.toast(str::kTvKeycodeInvalid);
                                     return;
                                 }
                                 app.settings().tvKeycode = keycode;
                                 app.saveSettings();
                                 app.applyTvSettings();
                                 app.toast(str::kSaved);
                             },
                             false, 8, str::kTvKeycodeHint));
                     },
                     nullptr, str::kTvKeycodeText});
    items.push_back({str::kTvMac,
                     [&app]() { return app.settings().tvMac.isEmpty() ? String(str::kNotSet) : app.settings().tvMac; },
                     [&app]() {
                         app.push(std::make_unique<TextFieldScreen>(
                             app, str::kTvMacPrompt, app.settings().tvMac,
                             [&app](const String& text) {
                                 uint8_t bytes[6];
                                 if (!text.isEmpty() && !parseMacAddress(text, bytes)) {
                                     app.toast(str::kTvMacInvalid);
                                     return;
                                 }
                                 app.settings().tvMac = text;
                                 app.saveSettings();
                                 app.applyTvSettings();
                                 app.toast(str::kSaved);
                             },
                             false, 17, str::kTvMacHint));
                     },
                     nullptr, str::kTvMacText});
    items.push_back({str::kTvForget, nullptr,
                     [&app]() {
                         app.push(std::make_unique<ConfirmScreen>(app, str::kTvForget, app.settings().tvHost, [&app]() {
                             app.settings().forgetTv();
                             app.saveSettings();
                             app.applyTvSettings();
                             app.toast(str::kSaved);
                         }));
                     },
                     nullptr, str::kTvForgetText});
    for (ListItem& item : items) {
        menu->items().push_back(std::move(item));
    }
    return menu;
}

std::unique_ptr<Screen> makeBleKeyTestMenu(App& app) {
    // webOS support for individual HID usages differs between models; this lets the user
    // find out which "Back"/"Home" variant their TV understands.
    struct Probe {
        const char* label;
        uint8_t keyboard;
        uint16_t consumer;
    };
    static constexpr Probe kProbes[] = {
        {"Escape", hid_key::kEscape, 0},
        {"Backspace", hid_key::kBackspace, 0},
        {"AC Back", 0, hid_consumer::kAcBack},
        {"AC Home", 0, hid_consumer::kAcHome},
        {"Keyboard Home", hid_key::kHome, 0},
        {"Menu key", hid_key::kMenu, 0},
        {"Consumer Menu", 0, hid_consumer::kMenu},
        {"Play/Pause", 0, hid_consumer::kPlayPause},
        {"Volume up", 0, hid_consumer::kVolumeUp},
    };
    std::vector<ListItem> items;
    for (const Probe& probe : kProbes) {
        items.push_back({probe.label, nullptr,
                         [&app, probe]() {
                             if (!app.ble().isConnected()) {
                                 app.toast(str::kBleNotConnected);
                                 return;
                             }
                             if (probe.keyboard != 0) {
                                 app.ble().sendRawKeyboard(probe.keyboard);
                             } else {
                                 app.ble().sendRawConsumer(probe.consumer);
                             }
                             app.toast(String(str::kSent) + probe.label);
                         },
                         nullptr, str::kBleKeyTestText});
    }
    return std::make_unique<ListScreen>(app, str::kBleKeyTest, std::move(items));
}

std::unique_ptr<Screen> makeBleMenu(App& app) {
    std::vector<ListItem> items;
    items.push_back({str::kBlePair, nullptr, [&app]() { app.push(std::make_unique<BlePairingScreen>(app)); }, nullptr,
                     str::kBlePairText});
    items.push_back(
        {str::kBleKeyTest, nullptr, [&app]() { app.push(makeBleKeyTestMenu(app)); }, nullptr, str::kBleKeyTestText});
    items.push_back({str::kBleForget, nullptr,
                     [&app]() {
                         app.push(std::make_unique<ConfirmScreen>(app, str::kBleForget, String(), [&app]() {
                             app.ble().begin();
                             app.ble().forgetBonds();
                             app.settings().bleSetupDone = false;
                             app.saveSettings();
                             app.toast(str::kSaved);
                         }));
                     },
                     nullptr, str::kBleForgetText});
    return std::make_unique<ListScreen>(app, str::kCardBleTitle, std::move(items));
}

std::unique_ptr<Screen> makeServiceMenu(App& app) {
    struct ServiceCode {
        const char* label;
        const char* description;
        RemoteKey key;
        bool risky;
    };
    static constexpr ServiceCode kCodes[] = {
        {"Power on", "Discrete IR code: only turns the TV on.", RemoteKey::PowerOn, false},
        {"Power off", "Discrete IR code: only turns the TV off.", RemoteKey::PowerOff, false},
        {"HDMI 1", "Switches straight to HDMI 1 (IR).", RemoteKey::Hdmi1, false},
        {"HDMI 2", "Switches straight to HDMI 2 (IR).", RemoteKey::Hdmi2, false},
        {"HDMI 3", "Switches straight to HDMI 3 (IR).", RemoteKey::Hdmi3, false},
        {"HDMI 4", "Switches straight to HDMI 4 (IR).", RemoteKey::Hdmi4, false},
        {"In Start (service)", "Factory service menu, password 0413 (else 0000). Can break the TV!",
         RemoteKey::InStart, true},
        {"EZ Adjust (service)", "Factory adjust menu, password 0413 (else 0000). Can break the TV!",
         RemoteKey::EzAdjust, true},
    };
    std::vector<ListItem> items;
    for (const ServiceCode& code : kCodes) {
        items.push_back({code.label, nullptr,
                         [&app, code]() {
                             if (!code.risky) {
                                 app.ir().sendKey(code.key);
                                 app.toast(String(str::kSent) + code.label);
                                 return;
                             }
                             // Service menus ask for a password and are driven by digits/arrows/OK:
                             // open the IR-only keypad so every key reaches the TV over infrared.
                             app.push(std::make_unique<ConfirmScreen>(app, code.label, str::kServiceWarning, [&app, code]() {
                                 app.ir().sendKey(code.key);
                                 app.push(std::make_unique<RemoteScreen>(app, true));
                             }));
                         },
                         nullptr, code.description});
    }
    return std::make_unique<ListScreen>(app, str::kServiceTitle, std::move(items));
}

std::unique_ptr<Screen> makeAboutMenu(App& app) {
    std::vector<ListItem> items;
    items.push_back({str::kAboutAuthor, []() { return String(str::kAuthor); },
                     [&app]() {
                         app.push(std::make_unique<InfoScreen>(app, str::kAuthor, str::kAuthorEmail,
                                                               String(str::kAuthorDetail) + " " + APP_VERSION));
                     },
                     nullptr, str::kAboutAuthorText});
    items.push_back({str::kAboutVersion, []() { return String(APP_VERSION); }, nullptr, nullptr, String()});
    items.push_back({str::kAboutDevice, [&app]() { return String(app.device().modelName()); }, nullptr, nullptr,
                     String()});
    items.push_back({str::kAboutImu, [&app]() { return onOff(app.device().hasImu()); }, nullptr, nullptr, String()});
    items.push_back({str::kAboutIp, [&app]() { return app.wifi().isConnected() ? app.wifi().localIp() : String("-"); },
                     nullptr, nullptr, String()});
    items.push_back(
        {str::kAboutHeap, []() { return String(ESP.getFreeHeap() / 1024) + " KB"; }, nullptr, nullptr, String()});
    return std::make_unique<ListScreen>(app, str::kAboutTitle, std::move(items));
}

}  // namespace

void selectTv(App& app, const String& host, const String& name, Screen* returnTo) {
    Settings& settings = app.settings();
    if (settings.tvHost != host) {
        settings.tvClientKey = "";  // a different TV needs its own pairing
        settings.tvMac = "";
    }
    settings.tvHost = host;
    settings.tvName = name;
    app.saveSettings();
    app.applyTvSettings();
    app.push(std::make_unique<TvPairingScreen>(app, returnTo));
}

void promptTvAddress(App& app, Screen* returnTo) {
    app.push(std::make_unique<TextFieldScreen>(
        app, str::kTvIpPrompt, app.settings().tvHost,
        [&app, returnTo](const String& text) {
            String host = text;
            host.trim();
            IPAddress address;
            if (!address.fromString(host)) {
                app.toast(str::kTvIpInvalid);
                return;
            }
            selectTv(app, host, String(), returnTo);
        },
        false, 15, str::kTvIpHint));
}

std::unique_ptr<Screen> makeMainMenu(App& app) {
    std::vector<ListItem> items;
    items.push_back(
        {str::kMenuAbout, nullptr, [&app]() { app.push(makeAboutMenu(app)); }, nullptr, str::kMenuAboutText});
    items.push_back({str::kMenuWifi,
                     [&app]() -> String {
                         if (app.settings().wifiSsid.isEmpty()) return String(str::kNotSet);
                         return app.settings().wifiSsid + (app.wifi().isConnected() ? "" : " (...)");
                     },
                     [&app]() { app.push(std::make_unique<WifiScanScreen>(app)); }, nullptr, str::kMenuWifiText});
    items.push_back({str::kMenuTv,
                     [&app]() -> String {
                         const Settings& s = app.settings();
                         if (s.tvHost.isEmpty()) return String(str::kNotSet);
                         return s.tvName.isEmpty() ? s.tvHost : s.tvName;
                     },
                     [&app]() { app.push(makeTvMenu(app)); }, nullptr, str::kMenuTvText});
    items.push_back({str::kMenuBluetooth,
                     [&app]() -> String {
                         if (!app.ble().started()) return String(str::kBleOff);
                         return String(app.ble().isConnected() ? str::kBleConnected : str::kBleWaiting);
                     },
                     [&app]() { app.push(makeBleMenu(app)); }, nullptr, str::kMenuBluetoothText});
    items.push_back({str::kMenuPointer, [&app]() { return String(app.settings().pointerSpeed) + " / 5"; }, nullptr,
                     [&app](int delta) {
                         Settings& s = app.settings();
                         s.pointerSpeed = constrain(s.pointerSpeed + delta, 1, 5);
                         app.saveSettings();
                         app.applyDisplaySettings();
                     },
                     str::kMenuPointerText});
    items.push_back({str::kMenuInvertY, [&app]() { return onOff(app.settings().pointerInvertY); }, nullptr,
                     [&app](int) {
                         app.settings().pointerInvertY = !app.settings().pointerInvertY;
                         app.saveSettings();
                         app.applyDisplaySettings();
                     },
                     str::kMenuInvertYText});
    items.push_back(
        {str::kMenuBrightness,
         [&app]() { return String(brightnessIndex(app.settings().brightness) + 1) + " / " + kBrightnessLevelCount; },
         nullptr,
         [&app](int delta) {
             const int index = constrain(static_cast<int>(brightnessIndex(app.settings().brightness)) + delta, 0,
                                         static_cast<int>(kBrightnessLevelCount) - 1);
             app.settings().brightness = kBrightnessLevels[index];
             app.saveSettings();
             app.applyDisplaySettings();
         },
         str::kMenuBrightnessText});
    items.push_back({str::kMenuKeyClick, [&app]() { return onOff(app.settings().keyClick); }, nullptr,
                     [&app](int) {
                         app.settings().keyClick = !app.settings().keyClick;
                         app.saveSettings();
                         app.keyClick();
                     },
                     str::kMenuKeyClickText});
    items.push_back({str::kMenuStatusLed, [&app]() { return onOff(app.settings().statusLed); }, nullptr,
                     [&app](int) {
                         app.settings().statusLed = !app.settings().statusLed;
                         app.saveSettings();
                         app.applyDisplaySettings();
                     },
                     str::kMenuStatusLedText});
    items.push_back({str::kMenuService, nullptr, [&app]() { app.push(makeServiceMenu(app)); }, nullptr,
                     str::kMenuServiceText});
    return std::make_unique<ListScreen>(app, str::kMenuTitle, std::move(items));
}

std::unique_ptr<Screen> makeAppsMenu(App& app) {
    std::vector<ListItem> items;
    for (const AppShortcut& shortcut : kApps) {
        const bool hasIr = shortcut.irCommand != 0;
        items.push_back({shortcut.title, [hasIr]() { return String(hasIr ? "IR" : ""); },
                         [&app, shortcut]() {
                             // Prefer the network (any installed app); IR covers the keys printed on LG remotes.
                             const bool network = app.settings().linkMode == LinkMode::Auto ||
                                                  app.settings().linkMode == LinkMode::Lan;
                             if (network && app.webos().isReady()) {
                                 app.webos().launchApp(shortcut.webOsId);
                             } else if (network && app.ipControl().isReady()) {
                                 app.ipControl().launchApp(shortcut.webOsId);
                             } else if (shortcut.irCommand != 0) {
                                 app.ir().sendRaw(shortcut.irCommand);
                             } else {
                                 app.toast(str::kTvNotConnected);
                                 return;
                             }
                             app.pop();
                             app.toast(String(str::kLaunching) + shortcut.title);
                         },
                         nullptr, hasIr ? str::kAppsIrText : str::kAppsLanText});
    }
    return std::make_unique<ListScreen>(app, str::kAppsTitle, std::move(items));
}

}  // namespace lgremote
