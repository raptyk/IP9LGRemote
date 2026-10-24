// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

#include "core/Transport.h"

namespace lgremote {

// Sends LG TV commands as NEC infrared frames (address 0x04) from the built-in IR LED.
class IrTransport : public Transport {
public:
    explicit IrTransport(uint8_t pin);

    void begin();

    const char* name() const override { return "IR"; }
    LinkState state() const override { return started_ ? LinkState::Ready : LinkState::Off; }
    bool supports(RemoteKey key) const override;
    bool sendKey(RemoteKey key) override;

    // Sends an arbitrary NEC command byte to the LG address (used by service codes).
    void sendRaw(uint8_t command, uint8_t repeats = 0);

private:
    uint8_t pin_;
    bool started_ = false;
    bool playing_ = false;  // LG IR has no play/pause toggle, we alternate PLAY and PAUSE
};

}  // namespace lgremote
