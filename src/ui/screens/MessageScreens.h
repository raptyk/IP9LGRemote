// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

#include <Arduino.h>

#include <functional>
#include <vector>

#include "net/WifiService.h"
#include "ui/Screen.h"
#include "ui/screens/ListScreen.h"

namespace lgremote {

// Yes / no question. Enter confirms, esc / del cancels.
class ConfirmScreen : public Screen {
public:
    ConfirmScreen(App& app, String question, String detail, std::function<void()> onConfirm,
                  std::function<void()> onCancel = nullptr);

    void onKey(const KeyEvent& event) override;
    void draw(M5Canvas& canvas, int top, int height) override;
    const char* footer() const override;

private:
    String question_;
    String detail_;
    std::function<void()> onConfirm_;
    std::function<void()> onCancel_;
};

// Small pop-up card with a title and up to two lines of text; any of enter/esc/del closes it.
class InfoScreen : public Screen {
public:
    InfoScreen(App& app, String title, String body, String detail = String());

    void onKey(const KeyEvent& event) override;
    void draw(M5Canvas& canvas, int top, int height) override;
    const char* footer() const override;

private:
    String title_;
    String body_;
    String detail_;
};

// Pairing with the TV over the network. Explains what to do on the TV for every state
// (prompt, PIN, failure with reason) and returns to `returnTo` once paired.
class TvPairingScreen : public Screen {
public:
    TvPairingScreen(App& app, Screen* returnTo);

    void onEnter() override;
    void onKey(const KeyEvent& event) override;
    void tick() override;
    void draw(M5Canvas& canvas, int top, int height) override;
    const char* footer() const override;

private:
    void askForPin();
    void switchMethod();

    Screen* returnTo_;
    uint32_t readySince_ = 0;
    bool pinAsked_ = false;
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
    TvSearchScreen(App& app, Screen* returnTo);

    void onEnter() override;
    void tick() override;

protected:
    void drawEmpty(M5Canvas& canvas, int top, int height) override;

private:
    void rebuild();

    Screen* returnTo_;
    bool started_ = false;
};

}  // namespace lgremote
