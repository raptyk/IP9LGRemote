// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#include "HelpScreen.h"

#include "app/App.h"
#include "ui/Strings.h"

namespace lgremote {

namespace {

// Two columns x five rows per page.
constexpr HelpPage kPages[] = {
    {"Navigation",
     {{"; . , /", "Arrows"},
      {"enter", "OK"},
      {"del", "Back"},
      {"esc", "Exit"},
      {"h", "Home"},
      {"s", "Settings"},
      {"q", "Quick menu"},
      {"i", "Input"},
      {"o", "Info"},
      {"e", "Guide"}}},
    {"TV & media",
     {{"p", "Power"},
      {"= -", "Volume"},
      {"m", "Mute"},
      {"] [", "Channel"},
      {"0-9", "Digits"},
      {"space", "Play/Pause"},
      {"z x c", "Rew/Stop/FF"},
      {"r g y b", "Colours"},
      {nullptr, nullptr},
      {nullptr, nullptr}}},
    {"Smart",
     {{"a / G0", "Pointer"},
      {"[ ]", "Scroll (ptr)"},
      {"t", "Type on TV"},
      {"l", "Apps"},
      {"opt", "App menu"},
      {"tab", "This help"},
      {"fn+esc", "App menu"},
      {nullptr, nullptr},
      {nullptr, nullptr},
      {nullptr, nullptr}}},
};

constexpr int kPageCount = sizeof(kPages) / sizeof(kPages[0]);

}  // namespace

HelpScreen::HelpScreen(App& app) : ip9::HelpScreen(app, str::kHelpTitle, kPages, kPageCount) {}

}  // namespace lgremote
