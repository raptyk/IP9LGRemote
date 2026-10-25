// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

#include "app/Settings.h"
#include "ui/Screen.h"

namespace lgremote {

// Start screen: a carousel of large icons (Wi-Fi remote, IR remote, Bluetooth, USB,
// Settings, Help). Each card explains itself and shows whether it is set up.
// Opening a mode that is not configured yet starts its setup checklist first.
class HomeScreen : public ip9::AppScreen<App, ip9::CarouselScreen> {
public:
    explicit HomeScreen(App& app);

    void onEnter() override;
    const char* footer() const override;
    // Own chip: the home screen is not tied to a link, so the status bar shows the app instead.
    const char* chipLabel() const override { return str::kChipHome; }

protected:
    CarouselCard card(int index) const override;
    void drawIcon(M5Canvas& canvas, int index, int cx, int cy, uint16_t colour, float scale,
                  uint16_t background) const override;
    void open(int index) override;
    void openHelp() override;
    // "s" opens the setup checklist of the selected mode even when it is set up.
    bool onExtraKey(const KeyEvent& event) override;
    // Red double blink whenever a card that still needs setup is shown.
    void onCardShown(int index) override;

private:
    void openCard(int index, bool forceSetup);
};

// Opens the remote for `mode`, running its setup checklist first if needed.
void openLinkMode(App& app, LinkMode mode, bool forceSetup);

}  // namespace lgremote
