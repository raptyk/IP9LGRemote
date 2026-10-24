// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

#include <Arduino.h>

#include <memory>

#include "ui/Screen.h"

namespace lgremote {

class App;

std::unique_ptr<Screen> makeMainMenu(App& app);
std::unique_ptr<Screen> makeAppsMenu(App& app);

// Stores the TV address, starts the network link and shows the pairing screen.
void selectTv(App& app, const String& host, const String& name);

}  // namespace lgremote
