// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#include "MessageScreens.h"

#include "app/App.h"
#include "app/Menus.h"
#include "ui/Screen.h"
#include "ui/Strings.h"
#include "ui/Theme.h"
#include "ui/screens/TextFieldScreen.h"

namespace lgremote {

namespace {
constexpr uint32_t kPairedCloseDelayMs = 1200;
constexpr int kAutoScanRounds = 3;
constexpr uint32_t kAutoScanDelayMs = 1500;
constexpr uint32_t kBleConnectedCloseDelayMs = 1500;
}  // namespace

// ---------------------------------------------------------------- TvPairingScreen

TvPairingScreen::TvPairingScreen(App& app, ip9::Screen* returnTo) : Screen(app), returnTo_(returnTo) {}

void TvPairingScreen::onEnter() {
    readySince_ = 0;
}

void TvPairingScreen::switchMethod() {
    Settings& settings = app_.settings();
    settings.tvUsePin = !settings.tvUsePin;
    settings.tvClientKey = "";
    app_.saveSettings();
    app_.applyTvSettings();
    pinAsked_ = false;
    app_.toast(settings.tvUsePin ? str::kPairMethodPin : str::kPairMethodPrompt);
}

void TvPairingScreen::askForPin() {
    pinAsked_ = true;
    App& app = app_;
    app.push(std::make_unique<TextFieldScreen>(
        app, str::kPairPinPrompt, String(), [&app](const String& pin) { app.webos().submitPin(pin); }, false, 8,
        str::kPairPinText));
}

void TvPairingScreen::onKey(const KeyEvent& event) {
    if (event.repeat) {
        return;
    }
    const WebOsStatus status = app_.webos().status();
    switch (event.code) {
        case 'p':
            switchMethod();
            return;
        case keycode::kEnter:
            if (status == WebOsStatus::WaitingForPin) {
                askForPin();
            } else if (status == WebOsStatus::Rejected || status == WebOsStatus::Unreachable) {
                app_.applyTvSettings();  // retry from scratch (shows the prompt again)
            }
            return;
        case keycode::kEsc:
        case keycode::kBackspace:
            app_.pop();
            return;
        default:
            return;
    }
}

void TvPairingScreen::tick() {
    WebOsClient& webos = app_.webos();
    const WebOsStatus status = webos.status();
    const bool failed = status == WebOsStatus::Rejected || status == WebOsStatus::Unreachable ||
                        status == WebOsStatus::NoNetwork;
    if (failed && !failureSignalled_) {
        app_.signalWarning();
    }
    failureSignalled_ = failed;
    if (status == WebOsStatus::Ready) {
        if (readySince_ == 0) {
            readySince_ = millis();
            webos.showToast(str::kPairToast);
        } else if (millis() - readySince_ > kPairedCloseDelayMs) {
            App& app = app_;
            app.popTo(returnTo_);  // `this` is retired here
            app.toast(str::kPairDone);
            return;
        }
    } else if (status == WebOsStatus::WaitingForPin && !pinAsked_) {
        askForPin();
        return;
    } else if (status != WebOsStatus::WaitingForPin) {
        pinAsked_ = false;
    }
    app_.invalidate();  // spinner animation
}

const char* TvPairingScreen::footer() const {
    switch (app_.webos().status()) {
        case WebOsStatus::WaitingForUser: return str::kFooterPairingAccept;
        case WebOsStatus::WaitingForPin: return str::kFooterPairingPin;
        case WebOsStatus::Rejected:
        case WebOsStatus::Unreachable: return str::kFooterPairingFailed;
        default: return str::kFooterPairing;
    }
}

void TvPairingScreen::draw(M5Canvas& canvas, int top, int height) {
    const WebOsStatus status = app_.webos().status();
    const String error = app_.webos().lastError();
    const String host = app_.settings().tvHost;
    const int spinnerSpace = 14;

    switch (status) {
        case WebOsStatus::Ready:
            painter::drawCenteredLines(canvas, top, height, str::kPairDone, app_.settings().tvName.c_str());
            break;
        case WebOsStatus::WaitingForUser:
            painter::drawCenteredLines(canvas, top, height - spinnerSpace, str::kPairAcceptTitle,
                                       str::kPairAcceptText);
            painter::drawSpinner(canvas, theme::kScreenWidth / 2, top + height - 8);
            break;
        case WebOsStatus::WaitingForPin:
            painter::drawCenteredLines(canvas, top, height, str::kPairPinTitle,
                                       error.isEmpty() ? str::kPairPinText : error.c_str());
            break;
        case WebOsStatus::Rejected:
            painter::drawCenteredLines(canvas, top, height, str::kPairFailedTitle, error.c_str(), str::kPairChecks);
            break;
        case WebOsStatus::Unreachable:
            painter::drawCenteredLines(canvas, top, height, str::kPairUnreachableTitle,
                                       (host + " - " + error).c_str(), str::kPairChecks);
            break;
        case WebOsStatus::NoNetwork:
            painter::drawCenteredLines(canvas, top, height, str::kPairNoWifiTitle, str::kPairNoWifiText);
            break;
        default:
            painter::drawCenteredLines(canvas, top, height - spinnerSpace, str::kPairConnectingTitle,
                                       host.c_str(), str::kPairConnectingText);
            painter::drawSpinner(canvas, theme::kScreenWidth / 2, top + height - 8);
            break;
    }
}

// ---------------------------------------------------------------- BlePairingScreen

void BlePairingScreen::onEnter() {
    if (app_.settings().linkMode != LinkMode::Ble) {
        app_.selectMode(LinkMode::Ble);
    }
    app_.ble().startPairing();
    wasConnected_ = app_.ble().isConnected();
    connectedAt_ = 0;
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
        connectedAt_ = millis();
        app_.toast(str::kBleConnected);
    }
    wasConnected_ = connected;
    if (connectedAt_ != 0 && millis() - connectedAt_ > kBleConnectedCloseDelayMs) {
        app_.pop();  // back to the checklist / menu
        return;
    }
    app_.invalidate();
}

const char* BlePairingScreen::footer() const {
    return str::kFooterPairing;
}

void BlePairingScreen::draw(M5Canvas& canvas, int top, int height) {
    using namespace theme;
    const int contentTop = painter::drawTitle(canvas, top, str::kBleTitle);
    canvas.setFont(&fonts::Font2);
    canvas.setTextSize(1);
    canvas.setTextDatum(top_left);
    canvas.setTextColor(kText);
    const int statusY = top + height - 9;
    const int lines = (statusY - 8 - contentTop) / 14;
    painter::drawWrappedText(canvas, str::kBleHowTo, kPadding, contentTop + 2, kScreenWidth - 2 * kPadding, 14,
                             lines);

    const bool connected = app_.ble().isConnected();
    canvas.setFont(&fonts::Font0);
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

bool WifiScanScreen::searching() const {
    return app_.wifi().scanInProgress() || nextAutoScanAt_ != 0;
}

void WifiScanScreen::restartSearch() {
    networks_.clear();
    emptyRounds_ = 0;
    nextAutoScanAt_ = 0;
    setItems({});
    app_.wifi().startScan();
}

void WifiScanScreen::tick() {
    if (app_.wifi().takeScanResults(networks_)) {
        // Right after boot the radio may still be busy joining the saved network and the
        // first scan comes back empty: keep searching (with a visible counter) before
        // telling the user that nothing was found.
        if (networks_.empty() && emptyRounds_ + 1 < kAutoScanRounds) {
            ++emptyRounds_;
            nextAutoScanAt_ = millis() + kAutoScanDelayMs;
        } else {
            rebuild();
        }
    }
    if (nextAutoScanAt_ != 0 && millis() >= nextAutoScanAt_) {
        nextAutoScanAt_ = 0;
        app_.wifi().startScan();
    }
    if (searching()) {
        app_.invalidate();
    }
}

void WifiScanScreen::rebuild() {
    std::vector<ListItem> items;
    for (const WifiNetwork& network : networks_) {
        WifiNetwork copy = network;
        String signal = String(network.rssi) + " dBm" + (network.secured ? " *" : "");
        items.push_back(
            {network.ssid, [signal]() { return signal; }, [this, copy]() { choose(copy); }, nullptr, str::kWifiPickText});
    }
    items.push_back({str::kWifiRescan, nullptr, [this]() { restartSearch(); }, nullptr,
                     networks_.empty() ? str::kWifiRescanText : str::kWifiRescanListText});
    if (networks_.empty()) {
        app_.signalWarning();
    }
    if (!app_.settings().wifiSsid.isEmpty()) {
        items.push_back({str::kWifiForget, nullptr,
                         [this]() {
                             app_.settings().wifiSsid = "";
                             app_.settings().wifiPassword = "";
                             app_.saveSettings();
                             app_.applyWifiSettings();
                             app_.pop();
                         },
                         nullptr, String()});
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
        app.toast(str::kWifiConnecting);
    };
    app.pop();  // close the network list; the password field (if any) opens in its place
    if (!network.secured) {
        connect("");
        return;
    }
    const String initial = ssid == app.settings().wifiSsid ? app.settings().wifiPassword : String();
    app.push(std::make_unique<TextFieldScreen>(app, String(str::kWifiPassword) + ": " + ssid, initial, connect, true));
}

void WifiScanScreen::drawEmpty(M5Canvas& canvas, int top, int height) {
    if (searching()) {
        char progress[48];
        snprintf(progress, sizeof(progress), str::kWifiStillScanning, emptyRounds_ + 1, kAutoScanRounds);
        painter::drawCenteredLines(canvas, top, height - 16, str::kWifiScanning,
                                   emptyRounds_ == 0 ? str::kWifiScanningText : progress);
        painter::drawSpinner(canvas, theme::kScreenWidth / 2, top + height - 10);
    } else {
        painter::drawCenteredLines(canvas, top, height, str::kWifiNoNetworks);
    }
}

// ---------------------------------------------------------------- TvSearchScreen

TvSearchScreen::TvSearchScreen(App& app, ip9::Screen* returnTo) : ListScreen(app, str::kTvSearch), returnTo_(returnTo) {}

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
            app_.warn(str::kTvNeedWifi);
        }
        rebuild();
    }
}

void TvSearchScreen::rebuild() {
    std::vector<ListItem> items;
    App& app = app_;
    ip9::Screen* returnTo = returnTo_;
    for (const DiscoveredTv& tv : app.discovery().results()) {
        const DiscoveredTv copy = tv;
        items.push_back({tv.name, [copy]() { return copy.host; },
                         [&app, copy, returnTo]() { selectTv(app, copy.host, copy.name, returnTo); }, nullptr,
                         str::kTvSearchResultText});
    }
    items.push_back({str::kTvManual, nullptr, [&app, returnTo]() { promptTvAddress(app, returnTo); }, nullptr,
                     str::kTvManualText});
    setItems(std::move(items));
}

void TvSearchScreen::drawEmpty(M5Canvas& canvas, int top, int height) {
    painter::drawCenteredLines(canvas, top, height - 16, str::kTvSearching);
    painter::drawSpinner(canvas, theme::kScreenWidth / 2, top + height - 10);
}

}  // namespace lgremote
