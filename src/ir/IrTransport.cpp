// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#include "IrTransport.h"

#include <Arduino.h>

#include "LgIrCodes.h"

// IRremote is a header-only library configured by macros; it must be included
// from exactly one translation unit.
#define DISABLE_CODE_FOR_RECEIVER
#define NO_LED_FEEDBACK_CODE
#include <IRremote.hpp>

namespace lgremote {

IrTransport::IrTransport(uint8_t pin) : pin_(pin) {}

void IrTransport::begin() {
    IrSender.begin(pin_);
    started_ = true;
}

bool IrTransport::supports(RemoteKey key) const {
    if (key == RemoteKey::PlayPause) {
        return true;
    }
    uint8_t command;
    return lg_ir::lookup(key, command);
}

bool IrTransport::sendKey(RemoteKey key) {
    if (!started_) {
        return false;
    }
    if (key == RemoteKey::PlayPause) {
        key = playing_ ? RemoteKey::Pause : RemoteKey::Play;
        playing_ = !playing_;
    }
    uint8_t command;
    if (!lg_ir::lookup(key, command)) {
        return false;
    }
    sendRaw(command);
    return true;
}

void IrTransport::sendRaw(uint8_t command, uint8_t repeats) {
    IrSender.sendNEC(lg_ir::kAddress, command, repeats);
}

}  // namespace lgremote
