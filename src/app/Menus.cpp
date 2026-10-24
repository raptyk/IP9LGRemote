// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#include "Menus.h"

#include "app/App.h"
#include "ble/HidReportMap.h"
#include "ui/Strings.h"
#include "ui/screens/ListScreen.h"
#include "ui/screens/MessageScreens.h"
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
    std::vector<ListItem> items;
    items.push_back({str::kTvSearch, nullptr, [&app]() { app.push(std::make_unique<TvSearchScreen>(app)); }, nullptr});
    items.push_back({str::kTvManual, [&app]() { return app.settings().tvHost.isEmpty() ? String(str::kNotSet) : app.settings().tvHost; },
                     [&app]() {
                         app.push(std::make_unique<TextFieldScreen>(
                             app, str::kTvIpPrompt, app.settings().tvHost,
                             [&app](const String& host) {
                                 IPAddress address;
                                 if (address.fromString(host)) {
                                     selectTv(app, host, String());
                                 }
                             },
                             false, 15));
                     },
                     nullptr});
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
                             false, 8));
                     },
                     nullptr});
    items.push_back({str::kTvMac, [&app]() { return app.settings().tvMac.isEmpty() ? String(str::kNotSet) : app.settings().tvMac; },
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
                             false, 17));
                     },
                     nullptr});
    items.push_back({str::kTvPairAgain, nullptr,
                     [&app]() {
                         if (app.settings().tvHost.isEmpty()) {
                             app.toast(str::kSetUpTvFirst);
                             return;
                         }
                         app.settings().tvClientKey = "";
                         app.saveSettings();
                         app.applyTvSettings();
                         app.push(std::make_unique<TvPairingScreen>(app));
                     },
                     nullptr});
    items.push_back({str::kTvForget, nullptr,
                     [&app]() {
                         app.push(std::make_unique<ConfirmScreen>(app, str::kTvForget, app.settings().tvHost, [&app]() {
                             app.settings().forgetTv();
                             app.saveSettings();
                             app.applyTvSettings();
                             app.toast(str::kSaved);
                         }));
                     },
                     nullptr});
    return std::make_unique<ListScreen>(app, str::kTvTitle, std::move(items));
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
                         nullptr});
    }
    return std::make_unique<ListScreen>(app, str::kBleKeyTest, std::move(items));
}

std::unique_ptr<Screen> makeBleMenu(App& app) {
    std::vector<ListItem> items;
    items.push_back({str::kBlePair, nullptr, [&app]() { app.push(std::make_unique<BlePairingScreen>(app)); }, nullptr});
    items.push_back({str::kBleKeyTest, nullptr, [&app]() { app.push(makeBleKeyTestMenu(app)); }, nullptr});
    items.push_back({str::kBleForget, nullptr,
                     [&app]() {
                         app.push(std::make_unique<ConfirmScreen>(app, str::kBleForget, String(), [&app]() {
                             app.ble().begin();
                             app.ble().forgetBonds();
                             app.toast(str::kSaved);
                         }));
                     },
                     nullptr});
    return std::make_unique<ListScreen>(app, str::kBleTitle, std::move(items));
}

std::unique_ptr<Screen> makeServiceMenu(App& app) {
    struct ServiceCode {
        const char* label;
        RemoteKey key;
        bool risky;
    };
    static constexpr ServiceCode kCodes[] = {
        {"Power on", RemoteKey::PowerOn, false},
        {"Power off", RemoteKey::PowerOff, false},
        {"HDMI 1", RemoteKey::Hdmi1, false},
        {"HDMI 2", RemoteKey::Hdmi2, false},
        {"HDMI 3", RemoteKey::Hdmi3, false},
        {"HDMI 4", RemoteKey::Hdmi4, false},
        {"In Start (service menu)", RemoteKey::InStart, true},
        {"EZ Adjust (service menu)", RemoteKey::EzAdjust, true},
    };
    std::vector<ListItem> items;
    for (const ServiceCode& code : kCodes) {
        items.push_back({code.label, nullptr,
                         [&app, code]() {
                             auto send = [&app, code]() {
                                 app.ir().sendKey(code.key);
                                 app.toast(String(str::kSent) + code.label);
                             };
                             if (code.risky) {
                                 app.push(std::make_unique<ConfirmScreen>(app, code.label, str::kServiceWarning, send));
                             } else {
                                 send();
                             }
                         },
                         nullptr});
    }
    return std::make_unique<ListScreen>(app, str::kServiceTitle, std::move(items));
}

std::unique_ptr<Screen> makeAboutMenu(App& app) {
    std::vector<ListItem> items;
    items.push_back({str::kAboutVersion, []() { return String(APP_VERSION); }, nullptr, nullptr});
    items.push_back({str::kAboutDevice, [&app]() { return String(app.device().modelName()); }, nullptr, nullptr});
    items.push_back({str::kAboutImu, [&app]() { return onOff(app.device().hasImu()); }, nullptr, nullptr});
    items.push_back({str::kAboutIp, [&app]() { return app.wifi().isConnected() ? app.wifi().localIp() : String("-"); },
                     nullptr, nullptr});
    items.push_back({str::kAboutHeap, []() { return String(ESP.getFreeHeap() / 1024) + " KB"; }, nullptr, nullptr});
    items.push_back({str::kAboutAuthor, []() { return String(str::kAuthor); }, nullptr, nullptr});
    return std::make_unique<ListScreen>(app, str::kAboutTitle, std::move(items));
}

}  // namespace

void selectTv(App& app, const String& host, const String& name) {
    Settings& settings = app.settings();
    if (settings.tvHost != host) {
        settings.tvClientKey = "";  // a different TV needs its own pairing
        settings.tvMac = "";
    }
    settings.tvHost = host;
    settings.tvName = name;
    if (settings.linkMode == LinkMode::Ir || settings.linkMode == LinkMode::Ble) {
        settings.linkMode = LinkMode::Auto;
        app.router().setMode(settings.linkMode);
    }
    app.saveSettings();
    app.applyTvSettings();
    app.push(std::make_unique<TvPairingScreen>(app));
}

std::unique_ptr<Screen> makeMainMenu(App& app) {
    std::vector<ListItem> items;
    items.push_back({str::kMenuWifi,
                     [&app]() -> String {
                         if (app.settings().wifiSsid.isEmpty()) return String(str::kNotSet);
                         return app.settings().wifiSsid + (app.wifi().isConnected() ? "" : " (...)");
                     },
                     [&app]() { app.push(std::make_unique<WifiScanScreen>(app)); }, nullptr});
    items.push_back({str::kMenuTv,
                     [&app]() -> String {
                         const Settings& s = app.settings();
                         if (s.tvHost.isEmpty()) return String(str::kNotSet);
                         return s.tvName.isEmpty() ? s.tvHost : s.tvName;
                     },
                     [&app]() { app.push(makeTvMenu(app)); }, nullptr});
    items.push_back({str::kMenuBluetooth,
                     [&app]() -> String {
                         if (!app.ble().started()) return String(str::kBleOff);
                         return String(app.ble().isConnected() ? str::kBleConnected : str::kBleWaiting);
                     },
                     [&app]() { app.push(makeBleMenu(app)); }, nullptr});
    items.push_back({str::kMenuPointer, [&app]() { return String(app.settings().pointerSpeed) + " / 5"; }, nullptr,
                     [&app](int delta) {
                         Settings& s = app.settings();
                         s.pointerSpeed = constrain(s.pointerSpeed + delta, 1, 5);
                         app.saveSettings();
                         app.applyDisplaySettings();
                     }});
    items.push_back({str::kMenuInvertY, [&app]() { return onOff(app.settings().pointerInvertY); }, nullptr,
                     [&app](int) {
                         app.settings().pointerInvertY = !app.settings().pointerInvertY;
                         app.saveSettings();
                         app.applyDisplaySettings();
                     }});
    items.push_back({str::kMenuBrightness,
                     [&app]() { return String(brightnessIndex(app.settings().brightness) + 1) + " / " + kBrightnessLevelCount; },
                     nullptr,
                     [&app](int delta) {
                         const int index = constrain(static_cast<int>(brightnessIndex(app.settings().brightness)) + delta, 0,
                                                     static_cast<int>(kBrightnessLevelCount) - 1);
                         app.settings().brightness = kBrightnessLevels[index];
                         app.saveSettings();
                         app.applyDisplaySettings();
                     }});
    items.push_back({str::kMenuKeyClick, [&app]() { return onOff(app.settings().keyClick); }, nullptr,
                     [&app](int) {
                         app.settings().keyClick = !app.settings().keyClick;
                         app.saveSettings();
                         app.keyClick();
                     }});
    items.push_back({str::kMenuService, nullptr, [&app]() { app.push(makeServiceMenu(app)); }, nullptr});
    items.push_back({str::kMenuAbout, nullptr, [&app]() { app.push(makeAboutMenu(app)); }, nullptr});
    return std::make_unique<ListScreen>(app, str::kMenuTitle, std::move(items));
}

std::unique_ptr<Screen> makeAppsMenu(App& app) {
    std::vector<ListItem> items;
    for (const AppShortcut& shortcut : kApps) {
        const bool hasIr = shortcut.irCommand != 0;
        items.push_back({shortcut.title, [hasIr]() { return String(hasIr ? "IR" : ""); },
                         [&app, shortcut]() {
                             // Prefer the network (any installed app); IR covers the keys printed on LG remotes.
                             if (app.settings().linkMode != LinkMode::Ir && app.webos().isReady()) {
                                 app.webos().launchApp(shortcut.webOsId);
                             } else if (app.settings().linkMode != LinkMode::Ir && app.ipControl().isReady()) {
                                 app.ipControl().launchApp(shortcut.webOsId);
                             } else if (shortcut.irCommand != 0) {
                                 app.ir().sendRaw(shortcut.irCommand);
                             } else {
                                 app.toast(str::kTvNotConnected);
                                 return;
                             }
                             app.popToRoot();
                             app.toast(String(str::kLaunching) + shortcut.title);
                         },
                         nullptr});
    }
    return std::make_unique<ListScreen>(app, str::kAppsTitle, std::move(items));
}

}  // namespace lgremote
