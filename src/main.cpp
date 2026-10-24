// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
//
// CardputerLGRemote - LG webOS TV remote for M5Stack Cardputer and Cardputer ADV.

#include <Arduino.h>

#include "app/App.h"

namespace {
lgremote::App app;
}

void setup() {
    app.begin();
}

void loop() {
    app.loop();
}
