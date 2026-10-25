// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#include "RemoteScreen.h"

#include "app/App.h"
#include "app/Menus.h"
#include "ui/Icons.h"
#include "ui/ModeStyle.h"
#include "ui/Painter.h"
#include "ui/Strings.h"
#include "ui/Theme.h"
#include "ui/screens/HelpScreen.h"
#include "ui/screens/TypingScreen.h"

namespace lgremote {

namespace {

constexpr uint32_t kFeedbackMs = 1500;
constexpr uint32_t kFlashMs = 120;
constexpr uint32_t kVolumeOverlayMs = 2500;
constexpr uint32_t kPointerTimeoutMs = 30000;
constexpr uint32_t kPointerFrameMs = 20;

struct KeyBinding {
    char code;
    RemoteKey key;
};

// Single-key bindings on the remote screen (see docs/UX_DESIGN.md).
constexpr KeyBinding kBindings[] = {
    {keycode::kUp, RemoteKey::Up},
    {keycode::kDown, RemoteKey::Down},
    {keycode::kLeft, RemoteKey::Left},
    {keycode::kRight, RemoteKey::Right},
    {keycode::kEnter, RemoteKey::Ok},
    {keycode::kBackspace, RemoteKey::Back},
    {keycode::kEsc, RemoteKey::Exit},
    {'h', RemoteKey::Home},
    {'s', RemoteKey::Settings},
    {'q', RemoteKey::QuickMenu},
    {'i', RemoteKey::Input},
    {'o', RemoteKey::Info},
    {'e', RemoteKey::Guide},
    {'p', RemoteKey::Power},
    {'=', RemoteKey::VolumeUp},
    {'-', RemoteKey::VolumeDown},
    {'m', RemoteKey::Mute},
    {']', RemoteKey::ChannelUp},
    {'[', RemoteKey::ChannelDown},
    {' ', RemoteKey::PlayPause},
    {'z', RemoteKey::Rewind},
    {'x', RemoteKey::Stop},
    {'c', RemoteKey::FastForward},
    {'r', RemoteKey::Red},
    {'g', RemoteKey::Green},
    {'y', RemoteKey::Yellow},
    {'b', RemoteKey::Blue},
};

RemoteKey bindingFor(char code) {
    if (code >= '0' && code <= '9') {
        return digitKey(code - '0');
    }
    for (const KeyBinding& binding : kBindings) {
        if (binding.code == code) {
            return binding.key;
        }
    }
    return RemoteKey::None;
}

bool isArrow(char code) {
    return code == keycode::kUp || code == keycode::kDown || code == keycode::kLeft || code == keycode::kRight;
}

uint16_t colourKeyColour(const String& label) {
    if (label == "RED") return theme::kKeyRed;
    if (label == "GREEN") return theme::kKeyGreen;
    if (label == "YELLOW") return theme::kKeyYellow;
    if (label == "BLUE") return theme::kKeyBlue;
    return theme::kText;
}

}  // namespace

RemoteScreen::RemoteScreen(App& app, bool infraredOnly) : Screen(app), infraredOnly_(infraredOnly) {}

void RemoteScreen::onKey(const KeyEvent& event) {
    if (handleAction(event)) {
        return;
    }
    if (pointerActive_ && handlePointerKey(event)) {
        return;
    }
    RemoteKey key = bindingFor(event.code);
    if (key == RemoteKey::None) {
        return;
    }
    // Power must never fire twice from a held key; other keys repeat only if it makes sense.
    if (event.repeat && !isRepeatableKey(key)) {
        return;
    }
    send(key);
}

bool RemoteScreen::handleAction(const KeyEvent& event) {
    if (event.repeat) {
        return false;
    }
    // Help on its own key: "?" needs shift + "/", and "/" is the right arrow, which matters more.
    // Fn is ignored for the arrows, so the Cardputer convention fn + ; . , / works as well.
    if (event.code == keycode::kTab || (event.code == keycode::kRight && event.shift) ||
        (event.code == 'h' && event.fn)) {
        app_.push(std::make_unique<HelpScreen>(app_));
        return true;
    }
    switch (event.code) {
        case keycode::kOpt:
            // Back to the home screen (mode choice, settings, help).
            stopPointer();
            app_.pop();
            return true;
        case keycode::kButtonG0:
        case 'a':
            if (infraredOnly_) {
                app_.warn(str::kNeedsLanOrBle);
            } else {
                togglePointer();
            }
            return true;
        case 't':
            if (infraredOnly_ || app_.router().textTransport() == nullptr) {
                app_.warn(app_.router().pointerUnavailableReason());
            } else {
                stopPointer();
                app_.push(std::make_unique<TypingScreen>(app_));
            }
            return true;
        case 'l':
            stopPointer();
            app_.push(makeAppsMenu(app_));
            return true;
        default:
            return false;
    }
}

bool RemoteScreen::handlePointerKey(const KeyEvent& event) {
    pointerLastActivityAt_ = millis();
    if (isArrow(event.code)) {
        return true;  // movement is handled continuously in tick()
    }
    if (event.code == keycode::kEnter) {
        if (!event.repeat && pointerLink_ != nullptr) {
            pointerLink_->click();
            app_.signalCommand(true);
            label_ = "CLICK";
            via_ = String(str::kVia) + pointerLink_->name();
            lastFailed_ = false;
            lastSentAt_ = millis();
            app_.keyClick();
        }
        return true;
    }
    if (event.code == ']' || event.code == '[') {
        // In pointer mode the channel keys act as the Magic Remote wheel.
        if (pointerLink_ != nullptr) {
            pointerLink_->scroll(event.code == ']' ? 1 : -1);
        }
        return true;
    }
    if (event.code == keycode::kEsc) {
        stopPointer();
        return true;
    }
    return false;
}

void RemoteScreen::send(RemoteKey key) {
    const uint32_t now = millis();
    Transport* used = nullptr;
    if (infraredOnly_) {
        used = app_.ir().sendKey(key) ? static_cast<Transport*>(&app_.ir()) : nullptr;
    } else {
        used = app_.router().sendKey(key);
    }
    app_.keyClick();
    app_.signalCommand(used != nullptr);

    if (isDigitKey(key)) {
        if (now - lastSentAt_ > kFeedbackMs || digits_.length() >= 4) {
            digits_ = "";
        }
        digits_ += remoteKeyLabel(key);
        label_ = digits_;
    } else {
        digits_ = "";
        label_ = remoteKeyLabel(key);
    }
    lastFailed_ = used == nullptr;
    via_ = used != nullptr ? String(str::kVia) + used->name() : String(str::kNotSent);
    lastWasVolume_ = key == RemoteKey::VolumeUp || key == RemoteKey::VolumeDown || key == RemoteKey::Mute;
    lastSentAt_ = now;
}

void RemoteScreen::togglePointer() {
    if (pointerActive_) {
        stopPointer();
        return;
    }
    Transport* link = app_.router().pointerTransport();
    if (link == nullptr) {
        app_.warn(app_.router().pointerUnavailableReason());
        return;
    }
    pointerActive_ = true;
    app_.setPointerActive(true);
    pointerLink_ = link;
    pointerLastActivityAt_ = millis();
    lastPointerTickAt_ = millis();
    app_.airMouse().start();
    // A tiny wiggle makes the TV show its cursor immediately.
    link->movePointer(1, 0);
    link->movePointer(-1, 0);
}

void RemoteScreen::stopPointer() {
    if (!pointerActive_) {
        return;
    }
    pointerActive_ = false;
    app_.setPointerActive(false);
    pointerLink_ = nullptr;
    app_.airMouse().stop();
    app_.invalidate();
}

void RemoteScreen::tick() {
    const uint32_t now = millis();
    if (!pointerActive_) {
        // Keep the feedback animation (flash, fade back to idle) moving.
        if (lastSentAt_ != 0 && now - lastSentAt_ < kFeedbackMs + 100) {
            app_.invalidate();
        }
        return;
    }
    if (pointerLink_ == nullptr || !pointerLink_->isReady()) {
        stopPointer();
        app_.warn(app_.router().pointerUnavailableReason());
        return;
    }

    int dx = 0;
    int dy = 0;
    if (app_.airMouse().poll(dx, dy)) {
        pointerLastActivityAt_ = now;
    }

    if (now - lastPointerTickAt_ >= kPointerFrameMs) {
        lastPointerTickAt_ = now;
        KeyboardInput& keys = app_.keyboard();
        const bool anyArrow = keys.isHeld(keycode::kUp) || keys.isHeld(keycode::kDown) ||
                              keys.isHeld(keycode::kLeft) || keys.isHeld(keycode::kRight);
        if (anyArrow) {
            if (arrowHeldSince_ == 0) {
                arrowHeldSince_ = now;
            }
            // Accelerate from precise 2 px steps to fast 16 px steps over ~1 s.
            const int step = min<int>(2 + (now - arrowHeldSince_) / 70, 16);
            dx += (keys.isHeld(keycode::kRight) ? step : 0) - (keys.isHeld(keycode::kLeft) ? step : 0);
            dy += (keys.isHeld(keycode::kDown) ? step : 0) - (keys.isHeld(keycode::kUp) ? step : 0);
            pointerLastActivityAt_ = now;
        } else {
            arrowHeldSince_ = 0;
        }
    }

    if (dx != 0 || dy != 0) {
        pointerLink_->movePointer(dx, dy);
    }
    if (now - pointerLastActivityAt_ < 300) {
        app_.invalidate();  // keeps the "moving" highlight of the crosshair in sync
    }
    if (now - pointerLastActivityAt_ > kPointerTimeoutMs) {
        stopPointer();
    }
}

const char* RemoteScreen::footer() const {
    if (infraredOnly_) {
        return str::kFooterServiceKeypad;
    }
    if (pointerActive_) {
        return str::kFooterPointer;
    }
    return shownMode() == LinkMode::Ir ? str::kFooterRemoteIr : str::kFooterRemote;
}

LinkMode RemoteScreen::shownMode() const {
    return infraredOnly_ ? LinkMode::Ir : app_.settings().linkMode;
}

// Layout of the content area (101 px):
//   band 24 px   - mode colour, icon, "WI-FI REMOTE" (+ "via IR" after a key press)
//   middle 52 px - hint, last command or pointer state
//   keys 25 px   - drawn key caps for the keys that matter most (help, home, pointer)
void RemoteScreen::draw(M5Canvas& canvas, int top, int height) {
    // A tinted background in the mode colour: the remote never looks like a menu.
    canvas.fillRect(0, top, theme::kScreenWidth, height, ui::modeTint(shownMode()));

    const bool feedback = !pointerActive_ && lastSentAt_ != 0 && millis() - lastSentAt_ < kFeedbackMs;
    drawBand(canvas, top, feedback);

    const int middleTop = top + kBandHeight;
    const int middleHeight = height - kBandHeight - kKeyRowHeight;
    if (pointerActive_) {
        drawPointer(canvas, middleTop, middleHeight);
    } else if (feedback) {
        drawFeedback(canvas, middleTop, middleHeight);
    } else {
        drawIdle(canvas, middleTop, middleHeight);
    }
    drawKeyRow(canvas, top + height - kKeyRowHeight, kKeyRowHeight);
}

void RemoteScreen::drawBand(M5Canvas& canvas, int top, bool showVia) {
    using namespace theme;
    const LinkMode mode = shownMode();
    canvas.fillRect(0, top, kScreenWidth, kBandHeight, ui::modeColour(mode));
    const int centerY = top + kBandHeight / 2;
    icons::draw(canvas, ui::modeIcon(mode), 16, centerY, kText, 0.45f);

    const char* title = ui::modeBandTitle(mode);
    if (infraredOnly_) {
        title = str::kBandService;
    } else if (pointerActive_) {
        title = app_.airMouse().available() ? str::kAirMouseTitle : str::kPointerTitle;
    }
    canvas.setFont(&fonts::FreeSansBold9pt7b);
    canvas.setTextSize(1);
    canvas.setTextColor(kText);
    canvas.setTextDatum(middle_left);
    painter::drawClippedText(canvas, title, 32, centerY + 1, showVia ? 140 : kScreenWidth - 40);

    if (showVia) {
        // Which link delivered the last command (fallbacks are never a mystery).
        canvas.setFont(&fonts::Font0);
        canvas.setTextDatum(middle_right);
        canvas.setTextColor(lastFailed_ ? kError : kText);
        canvas.drawString(via_, kScreenWidth - kPadding, centerY);
    }
}

void RemoteScreen::drawKeyRow(M5Canvas& canvas, int top, int height) {
    struct Hint {
        const char* key;
        const char* label;
    };
    Hint hints[3];
    int count = 0;
    if (pointerActive_) {
        hints[count++] = {str::kKeyEnter, str::kKeyClick};
        hints[count++] = {str::kKeyBrackets, str::kKeyScroll};
        hints[count++] = {str::kKeyA, str::kKeyExit};
    } else {
        hints[count++] = {str::kKeyTab, str::kKeyHelp};
        hints[count++] = {str::kKeyOpt, infraredOnly_ ? str::kKeyBack : str::kKeyHome};
        if (!infraredOnly_ && shownMode() != LinkMode::Ir) {
            hints[count++] = {str::kKeyA, app_.airMouse().available() ? str::kKeyAirMouse : str::kKeyPointer};
        }
    }

    // Spread the hints evenly over the row.
    int total = 0;
    for (int i = 0; i < count; ++i) {
        total += painter::measureKeyHint(canvas, hints[i].key, hints[i].label);
    }
    const int gap = (theme::kScreenWidth - total) / (count + 1);
    int x = gap;
    const int centerY = top + height / 2;
    for (int i = 0; i < count; ++i) {
        x += painter::drawKeyHint(canvas, x, centerY, hints[i].key, hints[i].label) + gap;
    }
}

String RemoteScreen::idleHint() const {
    if (infraredOnly_) {
        return str::kServiceKeypadText;
    }
    switch (shownMode()) {
        case LinkMode::Ir:
            return str::kHintIr;
        case LinkMode::Ble:
            return app_.ble().isConnected() ? str::kHintBleReady : str::kHintBleWaiting;
        case LinkMode::Usb:
            return app_.usb().isMounted() ? str::kHintUsbReady : str::kHintUsbWaiting;
        default: {
            if (!app_.webos().isReady() && !app_.ipControl().isReady()) {
                return str::kHintWifiOffline;
            }
            const Settings& settings = app_.settings();
            const String tv = settings.tvName.isEmpty() ? settings.tvHost : settings.tvName;
            return String(str::kHintWifiConnected) + tv + ". " + str::kHintNoAim;
        }
    }
}

void RemoteScreen::drawIdle(M5Canvas& canvas, int top, int height) {
    using namespace theme;
    const String hint = idleHint();
    const int lineHeight = 15;
    const int maxWidth = kScreenWidth - 2 * kPadding;
    canvas.setFont(&fonts::Font2);
    canvas.setTextSize(1);
    canvas.setTextColor(kText);
    canvas.setTextDatum(top_center);
    const int lines = min<int>(painter::wrapText(canvas, hint, maxWidth).size(), 3);
    const int y = top + (height - lines * lineHeight) / 2;
    painter::drawWrappedText(canvas, hint, kScreenWidth / 2, y, maxWidth, lineHeight, 3);
}

void RemoteScreen::drawFeedback(M5Canvas& canvas, int top, int height) {
    using namespace theme;
    const uint32_t age = millis() - lastSentAt_;
    const int centerX = kScreenWidth / 2;

    WebOsClient& webos = app_.webos();
    const int volume = webos.volume();
    const bool showVolume = lastWasVolume_ && volume >= 0 && millis() - webos.volumeChangedAt() < kVolumeOverlayMs;
    const int pillHeight = 34;
    const int pillTop = top + (height - pillHeight - (showVolume ? 10 : 0)) / 2;

    // Large font for short labels, a smaller one for long ones ("QUICK MENU") so nothing is cut.
    canvas.setTextSize(1);
    canvas.setFont(&fonts::FreeSansBold18pt7b);
    if (canvas.textWidth(label_) > kScreenWidth - 40) {
        canvas.setFont(&fonts::FreeSansBold12pt7b);
    }
    const int labelWidth = min<int>(canvas.textWidth(label_) + 28, kScreenWidth - 12);
    if (age < kFlashMs && !lastFailed_) {
        canvas.fillRoundRect(centerX - labelWidth / 2, pillTop, labelWidth, pillHeight, 10, kAccent);
    } else {
        canvas.drawRoundRect(centerX - labelWidth / 2, pillTop, labelWidth, pillHeight, 10,
                             lastFailed_ ? kError : kTextMuted);
    }
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(lastFailed_ ? kError : colourKeyColour(label_));
    painter::drawClippedText(canvas, label_, centerX, pillTop + pillHeight / 2 + 1, kScreenWidth - 24);

    // Volume level reported by the TV (network link only).
    if (showVolume) {
        const int barWidth = 150;
        const int barX = centerX - barWidth / 2 - 14;
        const int barY = pillTop + pillHeight + 5;
        canvas.fillRoundRect(barX, barY, barWidth, 5, 2, kSurfaceHigh);
        canvas.fillRoundRect(barX, barY, barWidth * constrain(volume, 0, 100) / 100, 5, 2, kAccent);
        canvas.setFont(&fonts::Font0);
        canvas.setTextColor(kText);
        canvas.setTextDatum(middle_left);
        canvas.drawString(String(volume), barX + barWidth + 8, barY + 3);
    }
}

void RemoteScreen::drawPointer(M5Canvas& canvas, int top, int height) {
    using namespace theme;
    const int centerY = top + height / 2;
    const int crossX = 44;

    // Crosshair "target" icon; it lights up while the cursor moves.
    const uint16_t colour = (millis() - pointerLastActivityAt_ < 200) ? kAccent : kText;
    canvas.drawCircle(crossX, centerY, 12, colour);
    canvas.drawCircle(crossX, centerY, 11, colour);
    canvas.fillCircle(crossX, centerY, 3, colour);
    canvas.drawFastHLine(crossX - 19, centerY, 8, colour);
    canvas.drawFastHLine(crossX + 12, centerY, 8, colour);
    canvas.drawFastVLine(crossX, centerY - 19, 8, colour);
    canvas.drawFastVLine(crossX, centerY + 12, 8, colour);

    const int textX = crossX + 30;
    const int maxWidth = kScreenWidth - textX - kPadding;
    const char* hint = app_.airMouse().available() ? str::kPointerHintAir : str::kPointerHintKeys;
    canvas.setFont(&fonts::Font2);
    canvas.setTextSize(1);
    canvas.setTextColor(kText);
    canvas.setTextDatum(top_left);
    const int lines = min<int>(painter::wrapText(canvas, hint, maxWidth).size(), 3);
    painter::drawWrappedText(canvas, hint, textX, centerY - lines * 15 / 2, maxWidth, 15, 3);
}

}  // namespace lgremote
