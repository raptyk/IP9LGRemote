// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

#include <Arduino.h>

#include <functional>
#include <vector>

#include "ui/Screen.h"
#include "ui/screens/ListScreen.h"
#include "net/WifiService.h"

namespace lgremote {

// Yes / no question. Enter confirms, esc / del cancels.
class ConfirmScreen : public Screen {
public:
    ConfirmScreen(App& app, String question, String detail, std::function<void()> onConfirm);

    void onKey(const KeyEvent& event) override;
    void draw(M5Canvas& canvas, int top, int height) override;
    const char* footer() const override;

private:
    String question_;
    String detail_;
    std::function<void()> onConfirm_;
};

// Shown while pairing with the TV over the network: explains the prompt on the TV
// and closes itself once the TV accepted.
class TvPairingScreen : public Screen {
public:
    using Screen::Screen;

    void onEnter() override;
    void onKey(const KeyEvent& event) override;
    void tick() override;
    void draw(M5Canvas& canvas, int top, int height) override;
    const char* footer() const override;

private:
    uint32_t readySince_ = 0;
};

// Step-by-step instructions for pairing the Bluetooth HID mode with the TV.
class BlePairingScreen : public Screen {
public:
    using Screen::Screen;

    void onEnter() override;
    void onKey(const KeyEvent& event) override;
    void tick() override;
    void draw(M5Canvas& canvas, int top, int height) override;
    const char* footer() const override;

private:
    bool wasConnected_ = false;
};

// Scans for Wi-Fi networks and asks for the password of the chosen one.
class WifiScanScreen : public ListScreen {
public:
    explicit WifiScanScreen(App& app);

    void onEnter() override;
    void tick() override;

protected:
    void drawEmpty(M5Canvas& canvas, int top, int height) override;

private:
    void rebuild();
    void choose(const WifiNetwork& network);

    std::vector<WifiNetwork> networks_;
    bool started_ = false;
};

// Searches the LAN for webOS TVs (SSDP) and offers manual IP entry.
class TvSearchScreen : public ListScreen {
public:
    explicit TvSearchScreen(App& app);

    void onEnter() override;
    void tick() override;

protected:
    void drawEmpty(M5Canvas& canvas, int top, int height) override;

private:
    void rebuild();
    bool started_ = false;
};

}  // namespace lgremote
