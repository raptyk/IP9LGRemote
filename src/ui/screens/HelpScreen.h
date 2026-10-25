// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

#include "ui/Screen.h"

namespace lgremote {

// Cheat sheet of the remote key map, split into pages (< > to flip).
class HelpScreen : public ip9::HelpScreen {
public:
    explicit HelpScreen(App& app);
};

}  // namespace lgremote
