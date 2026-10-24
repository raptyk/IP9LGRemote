// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

#include <Arduino.h>

#include <functional>
#include <vector>

#include "ui/Screen.h"

namespace lgremote {

struct ListItem {
    String label;
    std::function<String()> value;          // optional text on the right
    std::function<void()> onSelect;         // enter
    std::function<void(int delta)> onAdjust;  // left / right (for value settings)
};

// Vertical list navigated with ; . (up/down), enter to select, , / to adjust values,
// del or esc to go back. Used for menus and pickers.
class ListScreen : public Screen {
public:
    ListScreen(App& app, String title, std::vector<ListItem> items = {});

    void onKey(const KeyEvent& event) override;
    void draw(M5Canvas& canvas, int top, int height) override;
    const char* footer() const override;

protected:
    void setItems(std::vector<ListItem> items);
    // Content drawn instead of the list when it is empty (e.g. "Scanning...").
    virtual void drawEmpty(M5Canvas& canvas, int top, int height);

    String title_;
    std::vector<ListItem> items_;
    int selected_ = 0;
    int firstVisible_ = 0;
};

}  // namespace lgremote
