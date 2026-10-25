// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

#include <Arduino.h>

#include <memory>

#include "ui/Screen.h"

namespace lgremote {

class App;

std::unique_ptr<ip9::Screen> makeMainMenu(App& app);
std::unique_ptr<ip9::Screen> makeAppsMenu(App& app);

// Stores the TV address, restarts the network link and shows the pairing screen.
// After a successful pairing the screen stack returns to `returnTo`.
void selectTv(App& app, const String& host, const String& name, ip9::Screen* returnTo);

// Asks for the TV IP address, then continues like selectTv().
void promptTvAddress(App& app, ip9::Screen* returnTo);

}  // namespace lgremote
