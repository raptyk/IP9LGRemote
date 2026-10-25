// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
//
// IP9 LG Remote - LG webOS TV remote for M5Stack Cardputer and Cardputer ADV.
// Part of the IP9 apps: https://cardputer.ip9.pl

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
