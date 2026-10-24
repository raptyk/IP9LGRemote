// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#include "TextFieldScreen.h"

#include "app/App.h"
#include "ui/Painter.h"
#include "ui/Strings.h"
#include "ui/Theme.h"

namespace lgremote {

TextFieldScreen::TextFieldScreen(App& app, String title, String initial, SubmitHandler onSubmit, bool secret,
                                 size_t maxLength, String hint)
    : Screen(app),
      title_(std::move(title)),
      text_(std::move(initial)),
      hint_(std::move(hint)),
      onSubmit_(std::move(onSubmit)),
      secret_(secret),
      maxLength_(maxLength) {}

void TextFieldScreen::onKey(const KeyEvent& event) {
    switch (event.code) {
        case keycode::kEnter: {
            if (event.repeat) {
                return;
            }
            SubmitHandler handler = onSubmit_;
            String text = text_;
            app_.pop();
            if (handler) {
                handler(text);
            }
            return;
        }
        case keycode::kBackspace:
            if (event.fn && secret_) {
                reveal_ = !reveal_;
            } else if (text_.length() > 0) {
                text_.remove(text_.length() - 1);
            }
            return;
        case keycode::kEsc:
            // esc is also the "`" key: with shift it types "~", without it cancels.
            if (!event.shift) {
                app_.pop();
                return;
            }
            break;
        default:
            break;
    }
    if (event.text != 0 && text_.length() < maxLength_) {
        text_ += event.text;
    }
}

const char* TextFieldScreen::footer() const {
    return str::kFieldFooter;
}

void TextFieldScreen::draw(M5Canvas& canvas, int top, int height) {
    using namespace theme;
    const int contentTop = painter::drawTitle(canvas, top, title_.c_str());

    const int boxY = contentTop + 6;
    const int boxHeight = 30;
    canvas.fillRoundRect(kPadding, boxY, kScreenWidth - 2 * kPadding, boxHeight, 5, kSurface);
    canvas.drawRoundRect(kPadding, boxY, kScreenWidth - 2 * kPadding, boxHeight, 5, kAccent);

    String shown = text_;
    if (secret_ && !reveal_) {
        shown = "";
        for (size_t i = 0; i < text_.length(); ++i) {
            shown += '*';
        }
    }
    canvas.setFont(&fonts::FreeSans9pt7b);
    canvas.setTextSize(1);
    canvas.setTextColor(kText);
    canvas.setTextDatum(middle_left);

    // Keep the end of long text (where the caret is) visible.
    const int maxWidth = kScreenWidth - 2 * kPadding - 16;
    while (shown.length() > 0 && canvas.textWidth(shown) > maxWidth) {
        shown.remove(0, 1);
    }
    const int textX = kPadding + 8;
    const int textY = boxY + boxHeight / 2 + 1;
    canvas.drawString(shown, textX, textY);
    if ((millis() / 500) % 2 == 0) {
        const int caretX = textX + canvas.textWidth(shown) + 1;
        canvas.fillRect(caretX, boxY + 7, 2, boxHeight - 14, kAccent);
    }
    app_.invalidate();  // caret blink

    // Explanation under the field: what to type and where to find it.
    canvas.setFont(&fonts::Font0);
    canvas.setTextColor(kTextMuted);
    canvas.setTextDatum(top_left);
    const String hint = secret_ ? String(str::kRevealHint) : hint_;
    const int hintTop = boxY + boxHeight + 6;
    const int lines = max(0, (top + height - hintTop) / 10);
    painter::drawWrappedText(canvas, hint, kPadding, hintTop, kScreenWidth - 2 * kPadding, 10, lines);
}

}  // namespace lgremote
