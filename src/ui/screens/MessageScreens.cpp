// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#include "MessageScreens.h"

#include "app/App.h"
#include "app/Menus.h"
#include "ui/Painter.h"
#include "ui/Strings.h"
#include "ui/Theme.h"
#include "ui/screens/TextFieldScreen.h"

namespace lgremote {

namespace {
constexpr uint32_t kPairedCloseDelayMs = 1200;
}

// ---------------------------------------------------------------- ConfirmScreen

ConfirmScreen::ConfirmScreen(App& app, String question, String detail, std::function<void()> onConfirm)
    : Screen(app), question_(std::move(question)), detail_(std::move(detail)), onConfirm_(std::move(onConfirm)) {}

void ConfirmScreen::onKey(const KeyEvent& event) {
    if (event.repeat) {
        return;
    }
    if (event.code == keycode::kEnter || event.code == 'y') {
        std::function<void()> action = onConfirm_;
        app_.pop();
        if (action) {
            action();
        }
    } else if (event.code == keycode::kEsc || event.code == keycode::kBackspace || event.code == 'n') {
        app_.pop();
    }
}

const char* ConfirmScreen::footer() const {
    return str::kConfirmFooter;
}

void ConfirmScreen::draw(M5Canvas& canvas, int top, int height) {
    painter::drawCenteredLines(canvas, top, height, question_.c_str(), detail_.isEmpty() ? nullptr : detail_.c_str());
}

// ---------------------------------------------------------------- TvPairingScreen

void TvPairingScreen::onEnter() {
    readySince_ = 0;
}

void TvPairingScreen::onKey(const KeyEvent& event) {
    if (event.repeat) {
        return;
    }
    WebOsClient& webos = app_.webos();
    if (event.code == keycode::kEnter && webos.status() == WebOsStatus::Rejected) {
        app_.applyTvSettings();  // retry: shows the prompt on the TV again
        return;
    }
    if (event.code == keycode::kEsc || event.code == keycode::kBackspace || event.code == keycode::kEnter) {
        app_.popToRoot();
    }
}

void TvPairingScreen::tick() {
    WebOsClient& webos = app_.webos();
    if (webos.status() == WebOsStatus::Ready) {
        if (readySince_ == 0) {
            readySince_ = millis();
            webos.showToast(str::kPairToast);
        } else if (millis() - readySince_ > kPairedCloseDelayMs) {
            app_.popToRoot();
            app_.toast(str::kPairDone);
            return;
        }
    }
    app_.invalidate();  // spinner animation
}

const char* TvPairingScreen::footer() const {
    return app_.webos().status() == WebOsStatus::Rejected ? str::kConfirmFooter : str::kFooterList;
}

void TvPairingScreen::draw(M5Canvas& canvas, int top, int height) {
    const int contentTop = painter::drawTitle(canvas, top, str::kPairTitle);
    const int contentHeight = top + height - contentTop;
    const WebOsStatus status = app_.webos().status();

    switch (status) {
        case WebOsStatus::Ready:
            painter::drawCenteredLines(canvas, contentTop, contentHeight, str::kPairDone,
                                       app_.settings().tvName.c_str());
            break;
        case WebOsStatus::WaitingForUser:
            painter::drawCenteredLines(canvas, contentTop, contentHeight - 14, str::kPairAccept,
                                       app_.settings().tvHost.c_str());
            painter::drawSpinner(canvas, theme::kScreenWidth / 2, contentTop + contentHeight - 12);
            break;
        case WebOsStatus::Rejected:
            painter::drawCenteredLines(canvas, contentTop, contentHeight, str::kPairDeclined);
            break;
        default:
            painter::drawCenteredLines(canvas, contentTop, contentHeight - 14, str::kPairConnecting,
                                       webOsStatusText(status));
            painter::drawSpinner(canvas, theme::kScreenWidth / 2, contentTop + contentHeight - 12);
            break;
    }
}

// ---------------------------------------------------------------- BlePairingScreen

void BlePairingScreen::onEnter() {
    if (app_.settings().linkMode != LinkMode::Ble) {
        app_.settings().linkMode = LinkMode::Ble;
        app_.router().setMode(LinkMode::Ble);
        app_.saveSettings();
    }
    app_.ble().startPairing();
    wasConnected_ = app_.ble().isConnected();
}

void BlePairingScreen::onKey(const KeyEvent& event) {
    if (!event.repeat &&
        (event.code == keycode::kEsc || event.code == keycode::kBackspace || event.code == keycode::kEnter)) {
        app_.pop();
    }
}

void BlePairingScreen::tick() {
    const bool connected = app_.ble().isConnected();
    if (connected && !wasConnected_) {
        app_.toast(str::kBleConnected);
    }
    wasConnected_ = connected;
    app_.invalidate();
}

const char* BlePairingScreen::footer() const {
    return str::kFooterList;
}

void BlePairingScreen::draw(M5Canvas& canvas, int top, int height) {
    using namespace theme;
    const int contentTop = painter::drawTitle(canvas, top, str::kBleTitle);
    canvas.setFont(&fonts::Font0);
    canvas.setTextSize(1);
    canvas.setTextDatum(top_left);
    canvas.setTextColor(kText);
    canvas.drawString(str::kBleStep1, kPadding, contentTop + 6);
    canvas.drawString(str::kBleStep2, kPadding, contentTop + 18);
    canvas.drawString(str::kBleStep3, kPadding, contentTop + 30);

    const bool connected = app_.ble().isConnected();
    const int statusY = top + height - 16;
    canvas.fillCircle(kPadding + 4, statusY, 4, connected ? kOk : kWarn);
    canvas.setTextDatum(middle_left);
    canvas.setTextColor(connected ? kOk : kTextMuted);
    canvas.drawString(connected ? str::kBleConnected : str::kBleWaiting, kPadding + 14, statusY);
    if (!connected) {
        painter::drawSpinner(canvas, kScreenWidth - 40, statusY);
    }
}

// ---------------------------------------------------------------- WifiScanScreen

WifiScanScreen::WifiScanScreen(App& app) : ListScreen(app, str::kWifiTitle) {}

void WifiScanScreen::onEnter() {
    if (!started_) {
        started_ = true;
        app_.wifi().startScan();
    }
}

void WifiScanScreen::tick() {
    if (app_.wifi().takeScanResults(networks_)) {
        rebuild();
    }
    if (app_.wifi().scanInProgress()) {
        app_.invalidate();
    }
}

void WifiScanScreen::rebuild() {
    std::vector<ListItem> items;
    for (const WifiNetwork& network : networks_) {
        WifiNetwork copy = network;
        String signal = String(network.rssi) + " dBm" + (network.secured ? " *" : "");
        items.push_back({network.ssid, [signal]() { return signal; }, [this, copy]() { choose(copy); }, nullptr});
    }
    items.push_back({str::kWifiRescan, nullptr,
                     [this]() {
                         networks_.clear();
                         setItems({});
                         app_.wifi().startScan();
                     },
                     nullptr});
    if (!app_.settings().wifiSsid.isEmpty()) {
        items.push_back({str::kWifiForget, nullptr,
                         [this]() {
                             app_.settings().wifiSsid = "";
                             app_.settings().wifiPassword = "";
                             app_.saveSettings();
                             app_.applyWifiSettings();
                             app_.pop();
                         },
                         nullptr});
    }
    setItems(std::move(items));
}

void WifiScanScreen::choose(const WifiNetwork& network) {
    App& app = app_;
    const String ssid = network.ssid;
    auto connect = [&app, ssid](const String& password) {
        app.settings().wifiSsid = ssid;
        app.settings().wifiPassword = password;
        app.saveSettings();
        app.applyWifiSettings();
        app.popToRoot();
        app.toast(str::kWifiConnecting);
    };
    if (!network.secured) {
        connect("");
        return;
    }
    const String initial = ssid == app.settings().wifiSsid ? app.settings().wifiPassword : String();
    app.push(std::make_unique<TextFieldScreen>(app, String(str::kWifiPassword) + ": " + ssid, initial, connect, true));
}

void WifiScanScreen::drawEmpty(M5Canvas& canvas, int top, int height) {
    if (app_.wifi().scanInProgress()) {
        painter::drawCenteredLines(canvas, top, height - 16, str::kWifiScanning);
        painter::drawSpinner(canvas, theme::kScreenWidth / 2, top + height - 14);
    } else {
        painter::drawCenteredLines(canvas, top, height, str::kWifiNoNetworks);
    }
}

// ---------------------------------------------------------------- TvSearchScreen

TvSearchScreen::TvSearchScreen(App& app) : ListScreen(app, str::kTvSearch) {}

void TvSearchScreen::onEnter() {
    if (!started_) {
        started_ = true;
        app_.discovery().start();
    }
}

void TvSearchScreen::tick() {
    TvDiscovery& discovery = app_.discovery();
    if (discovery.running()) {
        if (!discovery.poll()) {
            rebuild();
        }
        app_.invalidate();
    } else if (items_.empty()) {
        // Search could not start (no Wi-Fi): offer manual entry right away.
        if (!app_.wifi().isConnected()) {
            app_.toast(str::kTvNeedWifi);
        }
        rebuild();
    }
}

void TvSearchScreen::rebuild() {
    std::vector<ListItem> items;
    App& app = app_;
    for (const DiscoveredTv& tv : app.discovery().results()) {
        const DiscoveredTv copy = tv;
        items.push_back({tv.name, [copy]() { return copy.host; },
                         [&app, copy]() { selectTv(app, copy.host, copy.name); }, nullptr});
    }
    items.push_back({str::kTvManual, nullptr,
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
    setItems(std::move(items));
}

void TvSearchScreen::drawEmpty(M5Canvas& canvas, int top, int height) {
    painter::drawCenteredLines(canvas, top, height - 16, str::kTvSearching);
    painter::drawSpinner(canvas, theme::kScreenWidth / 2, top + height - 14);
}

}  // namespace lgremote
