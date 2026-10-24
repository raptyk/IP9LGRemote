// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

#include <Arduino.h>

#include "core/RemoteKey.h"
#include "ui/Screen.h"

namespace lgremote {

class Transport;

// Main screen: every key press is a remote button; the screen confirms what was sent.
class RemoteScreen : public Screen {
public:
    using Screen::Screen;

    void onKey(const KeyEvent& event) override;
    void tick() override;
    void draw(M5Canvas& canvas, int top, int height) override;
    const char* footer() const override;

private:
    void send(RemoteKey key);
    bool handleAction(const KeyEvent& event);
    bool handlePointerKey(const KeyEvent& event);
    void togglePointer();
    void stopPointer();
    void drawIdle(M5Canvas& canvas, int top, int height);
    void drawFeedback(M5Canvas& canvas, int top, int height);
    void drawPointer(M5Canvas& canvas, int top, int height);

    String label_;
    String via_;
    bool lastFailed_ = false;
    bool lastWasVolume_ = false;
    uint32_t lastSentAt_ = 0;
    String digits_;

    bool pointerActive_ = false;
    Transport* pointerLink_ = nullptr;
    uint32_t pointerLastActivityAt_ = 0;
    uint32_t lastPointerTickAt_ = 0;
    uint32_t arrowHeldSince_ = 0;
};

}  // namespace lgremote
