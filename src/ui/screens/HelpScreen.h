// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

#include "ui/Screen.h"

namespace lgremote {

// Cheat sheet of the remote key map, split into pages (< > to flip).
class HelpScreen : public Screen {
public:
    using Screen::Screen;

    void onKey(const KeyEvent& event) override;
    void draw(M5Canvas& canvas, int top, int height) override;
    const char* footer() const override;

private:
    int page_ = 0;
};

}  // namespace lgremote
