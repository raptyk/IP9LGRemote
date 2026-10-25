// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#include "TypingScreen.h"

#include "app/App.h"
#include "ui/Painter.h"
#include "ui/Strings.h"
#include "ui/Theme.h"

namespace lgremote {

void TypingScreen::onKey(const KeyEvent& event) {
    Transport* link = app_.router().textTransport();
    if (link == nullptr) {
        app_.warn(app_.router().pointerUnavailableReason());
        app_.pop();
        return;
    }
    switch (event.code) {
        case keycode::kEnter:
            if (!event.repeat) {
                link->sendEnter();
                app_.pop();
            }
            return;
        case keycode::kBackspace:
            link->deleteText(1);
            if (echo_.length() > 0) {
                echo_.remove(echo_.length() - 1);
            }
            return;
        case keycode::kEsc:
            if (!event.shift) {
                app_.pop();
                return;
            }
            break;
        default:
            break;
    }
    if (event.text != 0) {
        const char text[2] = {event.text, '\0'};
        link->typeText(text);
        echo_ += event.text;
        app_.keyClick();
    }
}

const char* TypingScreen::footer() const {
    return str::kTypeFooter;
}

void TypingScreen::draw(M5Canvas& canvas, int top, int height) {
    using namespace theme;
    const int contentTop = painter::drawTitle(canvas, top, str::kTypeTitle);
    const int boxY = contentTop + 10;
    const int boxHeight = top + height - boxY - 8;
    canvas.fillRoundRect(kPadding, boxY, kScreenWidth - 2 * kPadding, boxHeight, 5, kSurface);

    canvas.setFont(&fonts::FreeSans9pt7b);
    canvas.setTextSize(1);
    canvas.setTextColor(kText);
    canvas.setTextDatum(middle_left);
    String shown = echo_;
    const int maxWidth = kScreenWidth - 2 * kPadding - 18;
    while (shown.length() > 0 && canvas.textWidth(shown) > maxWidth) {
        shown.remove(0, 1);
    }
    const int textY = boxY + boxHeight / 2;
    canvas.drawString(shown, kPadding + 8, textY);
    if ((millis() / 500) % 2 == 0) {
        canvas.fillRect(kPadding + 9 + canvas.textWidth(shown), textY - 8, 2, 16, kAccent);
    }
    app_.invalidate();  // caret blink
}

}  // namespace lgremote
