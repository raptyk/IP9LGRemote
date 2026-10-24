// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#include "SetupScreen.h"

#include "app/App.h"
#include "app/Menus.h"
#include "ui/Strings.h"
#include "ui/screens/MessageScreens.h"
#include "ui/screens/RemoteScreen.h"

namespace lgremote {

namespace {

const char* titleFor(LinkMode mode) {
    switch (mode) {
        case LinkMode::Ir: return str::kCardIrTitle;
        case LinkMode::Ble: return str::kCardBleTitle;
        case LinkMode::Usb: return str::kCardUsbTitle;
        default: return str::kCardWifiTitle;
    }
}

}  // namespace

SetupScreen::SetupScreen(App& app, LinkMode mode)
    : ListScreen(app, String(str::kSetupTitlePrefix) + titleFor(mode)), mode_(mode) {
    build();
}

void SetupScreen::onEnter() {
    // Called again when a step's screen closes: jump to the next open step.
    selectFirstOpenStep();
}

void SetupScreen::addStep(const char* label, const char* description, std::function<bool()> isDone,
                          std::function<void()> action, std::function<String()> value, bool required) {
    steps_.push_back({isDone, required});
    if (!value) {
        value = [isDone]() { return String(isDone() ? str::kStepDone : str::kStepTodo); };
    }
    items_.push_back({label, value, action, nullptr, description});
}

void SetupScreen::addStartRow() {
    items_.push_back({str::kStepStart, nullptr,
                      [this]() {
                          if (!allDone()) {
                              app_.toast(str::kStepFinishFirst);
                              return;
                          }
                          App& app = app_;
                          app.pop();  // replace the checklist with the remote
                          app.push(std::make_unique<RemoteScreen>(app));
                      },
                      nullptr, str::kStepStartText});
}

bool SetupScreen::allDone() const {
    for (const Step& step : steps_) {
        if (step.required && !step.isDone()) {
            return false;
        }
    }
    return true;
}

void SetupScreen::selectFirstOpenStep() {
    for (size_t i = 0; i < steps_.size(); ++i) {
        if (!steps_[i].isDone()) {
            select(static_cast<int>(i));
            return;
        }
    }
    select(static_cast<int>(items_.size()) - 1);  // everything done: "Start remote"
}

void SetupScreen::build() {
    App& app = app_;
    switch (mode_) {
        case LinkMode::Ir:
            addStep(str::kStepIrTest, str::kStepIrTestText, [&app]() { return app.settings().irSetupDone; },
                    [&app]() {
                        app.ir().sendKey(RemoteKey::VolumeUp);
                        app.push(std::make_unique<ConfirmScreen>(
                            app, str::kIrTestQuestion, str::kIrTestDetail,
                            [&app]() {
                                app.settings().irSetupDone = true;
                                app.saveSettings();
                            },
                            [&app]() { app.toast(str::kIrTestTips); }));
                    });
            break;

        case LinkMode::Ble:
            addStep(str::kStepBleCheck, str::kStepBleCheckText, [this]() { return tvPrepared_ || app_.settings().bleSetupDone; },
                    [this]() {
                        tvPrepared_ = true;
                        selectFirstOpenStep();
                    },
                    [this]() { return String(tvPrepared_ || app_.settings().bleSetupDone ? str::kStepOk : str::kStepTodo); });
            addStep(str::kStepBlePair, str::kStepBlePairText, [&app]() { return app.settings().bleSetupDone; },
                    [&app]() { app.push(std::make_unique<BlePairingScreen>(app)); });
            break;

        case LinkMode::Usb:
            addStep(str::kStepUsbCable, str::kStepUsbCableText, [&app]() { return app.settings().usbSetupDone; },
                    [&app]() { app.usb().begin(); },
                    [&app]() {
                        if (app.usb().isMounted()) return String(str::kBleConnected);
                        return String(app.usb().started() ? str::kUsbWaiting : str::kStepTodo);
                    });
            break;

        case LinkMode::Auto:
        case LinkMode::Lan:
        default:
            addStep(str::kStepWifi, str::kStepWifiText, [&app]() { return app.wifi().isConnected(); },
                    [&app]() { app.push(std::make_unique<WifiScanScreen>(app)); },
                    [&app]() -> String {
                        if (app.wifi().isConnected()) return app.settings().wifiSsid;
                        return app.settings().wifiSsid.isEmpty() ? String(str::kStepTodo) : String("...");
                    });
            addStep(str::kStepTvPrep, str::kStepTvPrepText,
                    [this]() { return tvPrepared_ || !app_.settings().tvClientKey.isEmpty(); },
                    [this]() {
                        tvPrepared_ = true;
                        selectFirstOpenStep();
                    },
                    [this]() {
                        return String(tvPrepared_ || !app_.settings().tvClientKey.isEmpty() ? str::kStepOk
                                                                                            : str::kStepTodo);
                    });
            addStep(str::kStepFindTv, str::kStepFindTvText, [&app]() { return !app.settings().tvHost.isEmpty(); },
                    [this]() {
                        if (!app_.wifi().isConnected()) {
                            app_.toast(str::kTvNeedWifi);
                            return;
                        }
                        app_.push(std::make_unique<TvSearchScreen>(app_, this));
                    },
                    [&app]() -> String {
                        const Settings& s = app.settings();
                        if (s.tvHost.isEmpty()) return String(str::kStepTodo);
                        return s.tvName.isEmpty() ? s.tvHost : s.tvName;
                    });
            addStep(str::kStepPair, str::kStepPairText, [&app]() { return app.settings().isSetUp(LinkMode::Auto); },
                    [this]() {
                        if (app_.settings().tvHost.isEmpty()) {
                            app_.toast(str::kSetUpTvFirst);
                            return;
                        }
                        app_.applyTvSettings();
                        app_.push(std::make_unique<TvPairingScreen>(app_, this));
                    });
            break;
    }
    addStartRow();
    selectFirstOpenStep();
}

}  // namespace lgremote
