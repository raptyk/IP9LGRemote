// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

#include "core/RemoteKey.h"

namespace lgremote::lg_ir {

// LG TVs use the NEC protocol with device address 0x04
// (32-bit LSB-first form 0x20DFxxxx, e.g. POWER = 0x20DF10EF).
// Cross-checked against Flipper-IRDB TVs/LG (~24 files), probonopd/irdb LG/TV/4,-1.csv,
// the LG owner's manual IR table and the Magic Remote BLE button codes (0x8000 | command).
// See docs/PROTOCOLS.md.
constexpr uint8_t kAddress = 0x04;

struct Code {
    RemoteKey key;
    uint8_t command;
};

constexpr Code kCodes[] = {
    {RemoteKey::Power, 0x08},
    {RemoteKey::PowerOn, 0xC4},
    {RemoteKey::PowerOff, 0xC5},
    {RemoteKey::Up, 0x40},
    {RemoteKey::Down, 0x41},
    {RemoteKey::Left, 0x07},
    {RemoteKey::Right, 0x06},
    {RemoteKey::Ok, 0x44},
    {RemoteKey::Back, 0x28},
    {RemoteKey::Exit, 0x5B},
    {RemoteKey::Home, 0x7C},
    {RemoteKey::Settings, 0x43},
    {RemoteKey::QuickMenu, 0x45},
    {RemoteKey::Input, 0x0B},
    {RemoteKey::Info, 0xAA},
    {RemoteKey::Guide, 0xAB},
    {RemoteKey::VolumeUp, 0x02},
    {RemoteKey::VolumeDown, 0x03},
    {RemoteKey::Mute, 0x09},
    {RemoteKey::ChannelUp, 0x00},
    {RemoteKey::ChannelDown, 0x01},
    {RemoteKey::Digit0, 0x10},
    {RemoteKey::Digit1, 0x11},
    {RemoteKey::Digit2, 0x12},
    {RemoteKey::Digit3, 0x13},
    {RemoteKey::Digit4, 0x14},
    {RemoteKey::Digit5, 0x15},
    {RemoteKey::Digit6, 0x16},
    {RemoteKey::Digit7, 0x17},
    {RemoteKey::Digit8, 0x18},
    {RemoteKey::Digit9, 0x19},
    {RemoteKey::Play, 0xB0},
    {RemoteKey::Pause, 0xBA},
    {RemoteKey::Stop, 0xB1},
    {RemoteKey::Rewind, 0x8F},
    {RemoteKey::FastForward, 0x8E},
    {RemoteKey::Red, 0x72},
    {RemoteKey::Green, 0x71},
    {RemoteKey::Yellow, 0x63},
    {RemoteKey::Blue, 0x61},
    {RemoteKey::Hdmi1, 0xCE},
    {RemoteKey::Hdmi2, 0xCC},
    {RemoteKey::Hdmi3, 0xE9},
    {RemoteKey::Hdmi4, 0xDA},
    {RemoteKey::EzAdjust, 0xFF},
    {RemoteKey::InStart, 0xFB},
};

// Returns true and sets `command` when the key has an IR code.
inline bool lookup(RemoteKey key, uint8_t& command) {
    for (const Code& code : kCodes) {
        if (code.key == key) {
            command = code.command;
            return true;
        }
    }
    return false;
}

}  // namespace lgremote::lg_ir
