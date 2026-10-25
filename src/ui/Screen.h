// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

#include <IP9Kit.h>

#include "ui/Strings.h"
#include "ui/Theme.h"

namespace lgremote {

// The shared IP9Kit core (https://github.com/raptyk/IP9Kit) is part of this app's vocabulary:
// KeyEvent, keycode::, painter::, ConfirmScreen, InfoScreen, ListItem, ...
using namespace ip9;

class App;

// Screen::traits() of this app.
namespace trait {
// The screen sends everything over infrared regardless of the selected mode (service keypad):
// the status bar and the LED then show the IR mode.
constexpr uint32_t kInfraredOnly = 1 << 0;
}  // namespace trait

// A full-screen view of this app; `app_` gives access to the App's services.
// Screens without their own chipLabel() show the link mode and its state in the status bar.
class Screen : public ip9::AppScreen<App> {
public:
    using AppScreen::AppScreen;
};

// A list (menus, setup checklists, scan results) with access to the App.
class ListScreen : public ip9::AppScreen<App, ip9::ListScreen> {
public:
    using AppScreen::AppScreen;
};

}  // namespace lgremote
