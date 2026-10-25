// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

#include "app/Settings.h"
#include "ui/Screen.h"

namespace lgremote {

// Start screen: a carousel of large icons (Wi-Fi remote, IR remote, Bluetooth, USB,
// Settings, Help). Each card explains itself and shows whether it is set up.
// Opening a mode that is not configured yet starts its setup checklist first.
class HomeScreen : public Screen {
public:
    explicit HomeScreen(App& app);

    void onEnter() override;
    void onKey(const KeyEvent& event) override;
    void draw(M5Canvas& canvas, int top, int height) override;
    const char* footer() const override;
    bool showsLinkStatus() const override { return false; }

private:
    void open(bool forceSetup);
    void move(int delta);
    // One red blink the first time (per session) a card that still needs setup is shown.
    void warnIfSetupNeeded();

    int selected_ = 0;
    int slideDirection_ = 0;  // -1 / +1 during the short slide animation
    uint32_t slideStartedAt_ = 0;
    uint32_t warnedCards_ = 0;  // bit per card
};

// Opens the remote for `mode`, running its setup checklist first if needed.
void openLinkMode(App& app, LinkMode mode, bool forceSetup);

}  // namespace lgremote
