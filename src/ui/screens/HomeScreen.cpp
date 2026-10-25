// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#include "HomeScreen.h"

#include "app/App.h"
#include "app/Menus.h"
#include "ui/Icons.h"
#include "ui/ModeStyle.h"
#include "ui/Strings.h"
#include "ui/Theme.h"
#include "ui/screens/HelpScreen.h"
#include "ui/screens/RemoteScreen.h"
#include "ui/screens/SetupScreen.h"

namespace lgremote {

namespace {

enum class CardKind : uint8_t { Mode, Settings, Help };

struct Card {
    icons::Icon icon;
    const char* title;
    const char* text;
    CardKind kind;
    LinkMode mode;
};

constexpr Card kCards[] = {
    {icons::Icon::Wifi, str::kCardWifiTitle, str::kCardWifiText, CardKind::Mode, LinkMode::Auto},
    {icons::Icon::Infrared, str::kCardIrTitle, str::kCardIrText, CardKind::Mode, LinkMode::Ir},
    {icons::Icon::Bluetooth, str::kCardBleTitle, str::kCardBleText, CardKind::Mode, LinkMode::Ble},
    {icons::Icon::Usb, str::kCardUsbTitle, str::kCardUsbText, CardKind::Mode, LinkMode::Usb},
    {icons::Icon::Settings, str::kCardSettingsTitle, str::kCardSettingsText, CardKind::Settings, LinkMode::Auto},
    {icons::Icon::Help, str::kCardHelpTitle, str::kCardHelpText, CardKind::Help, LinkMode::Auto},
};
constexpr int kCardCount = sizeof(kCards) / sizeof(kCards[0]);

}  // namespace

void openLinkMode(App& app, LinkMode mode, bool forceSetup) {
    app.selectMode(mode);
    if (!forceSetup && app.settings().isSetUp(mode)) {
        app.push(std::make_unique<RemoteScreen>(app));
    } else {
        app.push(std::make_unique<SetupScreen>(app, mode));
    }
}

HomeScreen::HomeScreen(App& app) : AppScreen(app, kCardCount) {}

void HomeScreen::onEnter() {
    // Start on the card of the last used mode.
    const LinkMode last = app_.settings().linkMode == LinkMode::Lan ? LinkMode::Auto : app_.settings().linkMode;
    for (int i = 0; i < kCardCount; ++i) {
        if (kCards[i].kind == CardKind::Mode && kCards[i].mode == last) {
            setSelected(i);
            break;
        }
    }
    onCardShown(selected());
}

void HomeScreen::onCardShown(int index) {
    // Every time such a card comes into view: one short red double blink, never a loop.
    const Card& card = kCards[index];
    if (card.kind == CardKind::Mode && !app_.settings().isSetUp(card.mode)) {
        app_.signalWarning();
    }
}

bool HomeScreen::onExtraKey(const KeyEvent& event) {
    if (event.code != 's') {
        return false;
    }
    if (!event.repeat) {
        openCard(selected(), true);
    }
    return true;
}

void HomeScreen::open(int index) {
    openCard(index, false);
}

void HomeScreen::openHelp() {
    app_.push(std::make_unique<HelpScreen>(app_));
}

void HomeScreen::openCard(int index, bool forceSetup) {
    const Card& card = kCards[index];
    switch (card.kind) {
        case CardKind::Mode:
            openLinkMode(app_, card.mode, forceSetup);
            break;
        case CardKind::Settings:
            app_.push(makeMainMenu(app_));
            break;
        case CardKind::Help:
            openHelp();
            break;
    }
}

const char* HomeScreen::footer() const {
    return str::kFooterHome;
}

void HomeScreen::drawIcon(M5Canvas& canvas, int index, int cx, int cy, uint16_t colour, float scale,
                          uint16_t background) const {
    icons::draw(canvas, kCards[index].icon, cx, cy, colour, scale, background);
}

CarouselCard HomeScreen::card(int index) const {
    using namespace theme;
    const Card& card = kCards[index];
    // Mode cards use the same colour as the status bar chip of that mode.
    CarouselCard result{card.title, card.text, card.kind == CardKind::Mode ? ui::modeColour(card.mode) : kSurface,
                        String(), kTextMuted};
    if (card.kind == CardKind::Mode) {
        // Status line: is this mode ready or does it need setup?
        const Settings& settings = app_.settings();
        const bool ready = settings.isSetUp(card.mode);
        result.status = ready ? String(str::kReady) : String(str::kSetupNeeded);
        if (ready && card.mode == LinkMode::Auto) {
            result.status += ": ";
            result.status += settings.tvName.isEmpty() ? settings.tvHost : settings.tvName;
        }
        result.statusColour = ready ? kOk : kWarn;
    }
    return result;
}

}  // namespace lgremote
