// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#include "RemoteScreen.h"

#include "app/App.h"
#include "app/Menus.h"
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
    // Help: "?" (shift + /) or fn + h.
    if ((event.code == keycode::kRight && event.shift) || (event.code == 'h' && event.fn)) {
        app_.push(std::make_unique<HelpScreen>(app_));
        return true;
    }
    if (event.fn && (event.code == keycode::kUp || event.code == keycode::kDown)) {
        Transport* link = app_.router().pointerTransport();
        if (link == nullptr) {
            app_.toast(app_.router().pointerUnavailableReason());
        } else {
            link->scroll(event.code == keycode::kUp ? -1 : 1);
        }
        return true;
    }
    switch (event.code) {
        case keycode::kTab:
            app_.cycleLinkMode();
            stopPointer();
            app_.toast(String(linkModeName(app_.settings().linkMode)) + " link");
            return true;
        case keycode::kOpt:
            stopPointer();
            app_.push(makeMainMenu(app_));
            return true;
        case keycode::kButtonG0:
        case 'a':
            togglePointer();
            return true;
        case 't':
            if (app_.router().textTransport() == nullptr) {
                app_.toast(app_.router().pointerUnavailableReason());
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
            label_ = "CLICK";
            via_ = String(str::kVia) + pointerLink_->name();
            lastFailed_ = false;
            lastSentAt_ = millis();
            app_.keyClick();
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
    Transport* used = app_.router().sendKey(key);
    app_.keyClick();

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
        app_.toast(app_.router().pointerUnavailableReason());
        return;
    }
    pointerActive_ = true;
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
        app_.toast(app_.router().pointerUnavailableReason());
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
        if (anyArrow && !keys.fnHeld()) {
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
    return pointerActive_ ? str::kFooterPointer : str::kFooterRemote;
}

void RemoteScreen::draw(M5Canvas& canvas, int top, int height) {
    if (pointerActive_) {
        drawPointer(canvas, top, height);
    } else if (lastSentAt_ != 0 && millis() - lastSentAt_ < kFeedbackMs) {
        drawFeedback(canvas, top, height);
    } else {
        drawIdle(canvas, top, height);
    }
}

void RemoteScreen::drawIdle(M5Canvas& canvas, int top, int height) {
    const Settings& settings = app_.settings();
    String title = settings.tvName.isEmpty() ? String(str::kIdleTitle) : settings.tvName;
    painter::drawCenteredLines(canvas, top, height, title.c_str(), str::kIdleHint);
}

void RemoteScreen::drawFeedback(M5Canvas& canvas, int top, int height) {
    using namespace theme;
    const uint32_t age = millis() - lastSentAt_;
    const int centerX = kScreenWidth / 2;
    const int centerY = top + height / 2 - 8;

    canvas.setFont(&fonts::FreeSansBold18pt7b);
    canvas.setTextSize(1);
    const int labelWidth = min<int>(canvas.textWidth(label_) + 28, kScreenWidth - 12);
    if (age < kFlashMs && !lastFailed_) {
        canvas.fillRoundRect(centerX - labelWidth / 2, centerY - 22, labelWidth, 44, 10, kAccent);
    } else {
        canvas.drawRoundRect(centerX - labelWidth / 2, centerY - 22, labelWidth, 44, 10,
                             lastFailed_ ? kError : kSurfaceHigh);
    }
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(lastFailed_ ? kError : colourKeyColour(label_));
    painter::drawClippedText(canvas, label_, centerX, centerY + 1, kScreenWidth - 24);

    // Volume level from the TV (network link only).
    WebOsClient& webos = app_.webos();
    const int volume = webos.volume();
    if (lastWasVolume_ && volume >= 0 && millis() - webos.volumeChangedAt() < kVolumeOverlayMs) {
        const int barWidth = 150;
        const int barX = centerX - barWidth / 2 - 14;
        const int barY = centerY + 30;
        canvas.fillRoundRect(barX, barY, barWidth, 6, 3, kSurfaceHigh);
        canvas.fillRoundRect(barX, barY, barWidth * constrain(volume, 0, 100) / 100, 6, 3, kAccent);
        canvas.setFont(&fonts::Font0);
        canvas.setTextColor(kText);
        canvas.setTextDatum(middle_left);
        canvas.drawString(String(volume), barX + barWidth + 8, barY + 3);
    } else {
        canvas.setFont(&fonts::Font0);
        canvas.setTextColor(kTextMuted);
        canvas.setTextDatum(middle_center);
        canvas.drawString(via_, centerX, centerY + 33);
    }
}

void RemoteScreen::drawPointer(M5Canvas& canvas, int top, int height) {
    using namespace theme;
    const int centerX = kScreenWidth / 2;
    const int centerY = top + height / 2 - 10;

    // Crosshair "target" icon.
    const uint16_t colour = (millis() - pointerLastActivityAt_ < 200) ? kAccent : kText;
    canvas.drawCircle(centerX, centerY, 16, colour);
    canvas.drawCircle(centerX, centerY, 15, colour);
    canvas.fillCircle(centerX, centerY, 3, colour);
    canvas.drawFastHLine(centerX - 24, centerY, 10, colour);
    canvas.drawFastHLine(centerX + 15, centerY, 10, colour);
    canvas.drawFastVLine(centerX, centerY - 24, 10, colour);
    canvas.drawFastVLine(centerX, centerY + 15, 10, colour);

    canvas.setFont(&fonts::FreeSansBold9pt7b);
    canvas.setTextSize(1);
    canvas.setTextColor(kText);
    canvas.setTextDatum(middle_center);
    canvas.drawString(str::kPointerTitle, centerX, centerY + 34);
    canvas.setFont(&fonts::Font0);
    canvas.setTextColor(kTextMuted);
    canvas.drawString(app_.airMouse().available() ? str::kPointerHintAir : str::kPointerHintKeys, centerX,
                      centerY + 48);
}

}  // namespace lgremote
