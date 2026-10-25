// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

#include <Arduino.h>

#include <functional>
#include <vector>

#include "net/WifiService.h"
#include "ui/Screen.h"

namespace lgremote {

// Pairing with the TV over the network. Explains what to do on the TV for every state
// (prompt, PIN, failure with reason) and returns to `returnTo` once paired.
class TvPairingScreen : public Screen {
public:
    TvPairingScreen(App& app, ip9::Screen* returnTo);

    void onEnter() override;
    void onKey(const KeyEvent& event) override;
    void tick() override;
    void draw(M5Canvas& canvas, int top, int height) override;
    const char* footer() const override;

private:
    void askForPin();
    void switchMethod();

    ip9::Screen* returnTo_;
    uint32_t readySince_ = 0;
    bool pinAsked_ = false;
    bool failureSignalled_ = false;  // one warning blink per failure, not a blinking loop
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
    uint32_t connectedAt_ = 0;
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
    bool searching() const;
    void restartSearch();
    void rebuild();
    void choose(const WifiNetwork& network);

    std::vector<WifiNetwork> networks_;
    bool started_ = false;
    int emptyRounds_ = 0;          // scans that came back empty during this search
    uint32_t nextAutoScanAt_ = 0;  // automatic follow-up scan, 0 = none pending
};

// Searches the LAN for webOS TVs (SSDP) and offers manual IP entry.
class TvSearchScreen : public ListScreen {
public:
    TvSearchScreen(App& app, ip9::Screen* returnTo);

    void onEnter() override;
    void tick() override;

protected:
    void drawEmpty(M5Canvas& canvas, int top, int height) override;

private:
    void rebuild();

    ip9::Screen* returnTo_;
    bool started_ = false;
};

}  // namespace lgremote
