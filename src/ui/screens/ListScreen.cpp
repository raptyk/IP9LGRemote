// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#include "ListScreen.h"

#include "app/App.h"
#include "ui/Painter.h"
#include "ui/Strings.h"
#include "ui/Theme.h"

namespace lgremote {

namespace {
constexpr int kRowHeight = 20;
}

ListScreen::ListScreen(App& app, String title, std::vector<ListItem> items)
    : Screen(app), title_(std::move(title)), items_(std::move(items)) {}

void ListScreen::setItems(std::vector<ListItem> items) {
    items_ = std::move(items);
    selected_ = constrain(selected_, 0, max(0, static_cast<int>(items_.size()) - 1));
    firstVisible_ = min(firstVisible_, selected_);
    app_.invalidate();
}

void ListScreen::onKey(const KeyEvent& event) {
    const int count = static_cast<int>(items_.size());
    switch (event.code) {
        case keycode::kUp:
            if (count > 0) {
                selected_ = (selected_ + count - 1) % count;
            }
            break;
        case keycode::kDown:
            if (count > 0) {
                selected_ = (selected_ + 1) % count;
            }
            break;
        case keycode::kLeft:
        case keycode::kRight:
            if (count > 0 && items_[selected_].onAdjust) {
                items_[selected_].onAdjust(event.code == keycode::kRight ? 1 : -1);
            }
            break;
        case keycode::kEnter:
            if (!event.repeat && count > 0) {
                ListItem& item = items_[selected_];
                if (item.onSelect) {
                    item.onSelect();  // may push/pop screens; `this` can be retired afterwards
                } else if (item.onAdjust) {
                    item.onAdjust(1);
                }
            }
            return;
        case keycode::kBackspace:
        case keycode::kEsc:
            if (!event.repeat) {
                app_.pop();
            }
            return;
        default:
            // Digit shortcuts: 1..9 select and activate an item directly.
            if (!event.repeat && event.code >= '1' && event.code <= '9') {
                const int index = event.code - '1';
                if (index < count) {
                    selected_ = index;
                    if (items_[index].onSelect) {
                        items_[index].onSelect();
                        return;
                    }
                }
            }
            break;
    }
}

const char* ListScreen::footer() const {
    if (!items_.empty() && items_[selected_].onAdjust && !items_[selected_].onSelect) {
        return str::kFooterValue;
    }
    return str::kFooterList;
}

void ListScreen::draw(M5Canvas& canvas, int top, int height) {
    const int listTop = painter::drawTitle(canvas, top, title_.c_str());
    const int listHeight = top + height - listTop;
    if (items_.empty()) {
        drawEmpty(canvas, listTop, listHeight);
        return;
    }

    const int visible = max(1, listHeight / kRowHeight);
    if (selected_ < firstVisible_) {
        firstVisible_ = selected_;
    } else if (selected_ >= firstVisible_ + visible) {
        firstVisible_ = selected_ - visible + 1;
    }

    for (int row = 0; row < visible; ++row) {
        const int index = firstVisible_ + row;
        if (index >= static_cast<int>(items_.size())) {
            break;
        }
        const ListItem& item = items_[index];
        const String value = item.value ? item.value() : String();
        painter::drawListRow(canvas, listTop + row * kRowHeight, kRowHeight - 1, item.label, value,
                             index == selected_);
    }
    painter::drawScrollBar(canvas, listTop, listHeight, firstVisible_, visible, items_.size());
}

void ListScreen::drawEmpty(M5Canvas& canvas, int top, int height) {
    painter::drawCenteredLines(canvas, top, height, "-");
}

}  // namespace lgremote
