// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

#include <Arduino.h>

#include "app/Settings.h"
#include "core/RemoteKey.h"
#include "ui/Screen.h"

namespace lgremote {

class Transport;

// Main screen: every key press is a remote button; the screen confirms what was sent.
// With `infraredOnly` it becomes the service keypad: every key goes out over IR
// (the factory menus and their password prompt only react to infrared).
class RemoteScreen : public Screen {
public:
    explicit RemoteScreen(App& app, bool infraredOnly = false);

    void onKey(const KeyEvent& event) override;
    void tick() override;
    void draw(M5Canvas& canvas, int top, int height) override;
    const char* footer() const override;
    bool forcesInfrared() const override { return infraredOnly_; }

private:
    void send(RemoteKey key);
    bool handleAction(const KeyEvent& event);
    bool handlePointerKey(const KeyEvent& event);
    void togglePointer();
    void stopPointer();
    static constexpr int kBandHeight = 24;
    static constexpr int kKeyRowHeight = 25;

    LinkMode shownMode() const;
    enum class StatusKind : uint8_t { Ok, Waiting, Error, Info };
    struct ConnectionStatus {
        StatusKind kind;
        String title;
        String detail;
    };

    // What the idle screen says about the link: green check = connected, amber = connecting,
    // red = not connected (keys fall back to IR), mode colour = infrared (nothing to connect).
    ConnectionStatus connectionStatus() const;
    void drawStatusBadge(M5Canvas& canvas, int cx, int cy, StatusKind kind);
    void drawBand(M5Canvas& canvas, int top, bool showVia);
    void drawKeyRow(M5Canvas& canvas, int top, int height);
    void drawIdle(M5Canvas& canvas, int top, int height);
    void drawFeedback(M5Canvas& canvas, int top, int height);
    void drawPointer(M5Canvas& canvas, int top, int height);

    bool infraredOnly_;
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
