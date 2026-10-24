// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

#include <Arduino.h>

#include <functional>

#include "ui/Screen.h"

namespace lgremote {

// Single-line text input (Wi-Fi password, IP address, keycode).
// Uses the full keyboard: shift for capitals, del erases, enter saves, esc cancels.
class TextFieldScreen : public Screen {
public:
    using SubmitHandler = std::function<void(const String& text)>;

    TextFieldScreen(App& app, String title, String initial, SubmitHandler onSubmit, bool secret = false,
                    size_t maxLength = 64);

    void onKey(const KeyEvent& event) override;
    void draw(M5Canvas& canvas, int top, int height) override;
    const char* footer() const override;

private:
    String title_;
    String text_;
    SubmitHandler onSubmit_;
    bool secret_;
    bool reveal_ = false;  // fn + del toggles password visibility
    size_t maxLength_;
};

}  // namespace lgremote
