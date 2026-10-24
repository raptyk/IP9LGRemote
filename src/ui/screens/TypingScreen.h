// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

#include <Arduino.h>

#include "ui/Screen.h"

namespace lgremote {

// Live typing into the focused text field on the TV: every character is sent as it is typed.
class TypingScreen : public Screen {
public:
    using Screen::Screen;

    void onKey(const KeyEvent& event) override;
    void draw(M5Canvas& canvas, int top, int height) override;
    const char* footer() const override;

private:
    String echo_;  // local echo of what was sent during this session
};

}  // namespace lgremote
