// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#include "HomeScreen.h"

#include "app/App.h"
#include "app/Menus.h"
#include "ui/Icons.h"
#include "ui/ModeStyle.h"
#include "ui/Painter.h"
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
constexpr uint32_t kSlideMs = 140;

}  // namespace

void openLinkMode(App& app, LinkMode mode, bool forceSetup) {
    app.selectMode(mode);
    if (!forceSetup && app.settings().isSetUp(mode)) {
        app.push(std::make_unique<RemoteScreen>(app));
    } else {
        app.push(std::make_unique<SetupScreen>(app, mode));
    }
}

HomeScreen::HomeScreen(App& app) : Screen(app) {}

void HomeScreen::onEnter() {
    // Start on the card of the last used mode.
    const LinkMode last = app_.settings().linkMode == LinkMode::Lan ? LinkMode::Auto : app_.settings().linkMode;
    for (int i = 0; i < kCardCount; ++i) {
        if (kCards[i].kind == CardKind::Mode && kCards[i].mode == last) {
            selected_ = i;
            break;
        }
    }
}

void HomeScreen::move(int delta) {
    selected_ = (selected_ + delta + kCardCount) % kCardCount;
    slideDirection_ = delta;
    slideStartedAt_ = millis();
    app_.invalidate();
}

void HomeScreen::onKey(const KeyEvent& event) {
    switch (event.code) {
        case keycode::kLeft:
        case keycode::kUp:
            move(-1);
            return;
        case keycode::kRight:
            if (event.shift) {
                app_.push(std::make_unique<HelpScreen>(app_));  // "?"
                return;
            }
            move(1);
            return;
        case keycode::kDown:
            move(1);
            return;
        case keycode::kTab:
            if (!event.repeat) {
                app_.push(std::make_unique<HelpScreen>(app_));
            }
            return;
        case keycode::kEnter:
        case keycode::kButtonG0:
            if (!event.repeat) {
                open(false);
            }
            return;
        case 's':
            if (!event.repeat) {
                open(true);
            }
            return;
        default:
            if (!event.repeat && event.code >= '1' && event.code < '1' + kCardCount) {
                selected_ = event.code - '1';
                open(false);
            }
            return;
    }
}

void HomeScreen::open(bool forceSetup) {
    const Card& card = kCards[selected_];
    switch (card.kind) {
        case CardKind::Mode:
            openLinkMode(app_, card.mode, forceSetup);
            break;
        case CardKind::Settings:
            app_.push(makeMainMenu(app_));
            break;
        case CardKind::Help:
            app_.push(std::make_unique<HelpScreen>(app_));
            break;
    }
}

const char* HomeScreen::footer() const {
    return str::kFooterHome;
}

void HomeScreen::draw(M5Canvas& canvas, int top, int height) {
    using namespace theme;
    const Card& card = kCards[selected_];
    const int centerX = kScreenWidth / 2;

    // Short slide-in of the selected icon.
    int offset = 0;
    const uint32_t age = millis() - slideStartedAt_;
    if (slideDirection_ != 0 && age < kSlideMs) {
        offset = slideDirection_ * static_cast<int>(30 * (kSlideMs - age) / kSlideMs);
        app_.invalidate();
    }

    // Neighbour previews and chevrons hint that the list scrolls sideways.
    const int iconY = top + 24;
    const Card& previous = kCards[(selected_ + kCardCount - 1) % kCardCount];
    const Card& next = kCards[(selected_ + 1) % kCardCount];
    icons::draw(canvas, previous.icon, 34, iconY, kSurfaceHigh, 0.55f);
    icons::draw(canvas, next.icon, kScreenWidth - 34, iconY, kSurfaceHigh, 0.55f);
    canvas.fillTriangle(6, iconY, 12, iconY - 6, 12, iconY + 6, kTextMuted);
    canvas.fillTriangle(kScreenWidth - 6, iconY, kScreenWidth - 12, iconY - 6, kScreenWidth - 12, iconY + 6,
                        kTextMuted);

    // Mode cards use the same colour as the status bar chip of that mode.
    const uint16_t cardColour = card.kind == CardKind::Mode ? ui::modeColour(card.mode) : kSurface;
    canvas.fillRoundRect(centerX - 26 + offset, iconY - 22, 52, 44, 10, cardColour);
    icons::draw(canvas, card.icon, centerX + offset, iconY, kText, 1.0f);

    canvas.setTextSize(1);
    canvas.setFont(&fonts::FreeSansBold9pt7b);
    canvas.setTextColor(kText);
    canvas.setTextDatum(top_center);
    painter::drawClippedText(canvas, card.title, centerX, top + 49, kScreenWidth - 2 * kPadding);

    // Status line: is this mode ready or does it need setup?
    canvas.setFont(&fonts::Font0);
    int textTop = top + 66;
    if (card.kind == CardKind::Mode) {
        const bool ready = app_.settings().isSetUp(card.mode);
        String status = ready ? String(str::kReady) : String(str::kSetupNeeded);
        if (ready && card.mode == LinkMode::Auto) {
            const Settings& settings = app_.settings();
            status += ": ";
            status += settings.tvName.isEmpty() ? settings.tvHost : settings.tvName;
        }
        const uint16_t colour = ready ? kOk : kWarn;
        const int width = min<int>(canvas.textWidth(status), kScreenWidth - 40);
        const int x = centerX - width / 2;
        canvas.fillCircle(x - 7, textTop + 3, 3, colour);
        canvas.setTextColor(colour);
        canvas.setTextDatum(top_left);
        painter::drawClippedText(canvas, status, x, textTop, kScreenWidth - 40);
        textTop += 12;
    }
    canvas.setTextColor(kTextMuted);
    canvas.setTextDatum(top_center);
    const int lines = (top + height - textTop) / 10;
    painter::drawWrappedText(canvas, card.text, centerX, textTop, kScreenWidth - 2 * kPadding - 8, 10, lines);
}

}  // namespace lgremote
