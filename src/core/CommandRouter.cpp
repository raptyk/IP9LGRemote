// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#include "CommandRouter.h"

#include "ble/BleHidTransport.h"
#include "ipcontrol/IpControlClient.h"
#include "ir/IrTransport.h"
#include "ui/Strings.h"
#include "webos/WakeOnLan.h"
#include "webos/WebOsClient.h"

namespace lgremote {

namespace {

// Keys that AUTO mode always sends over IR: they must work while the TV is off,
// or the IR code is more direct than any network equivalent.
bool isIrPreferredInAuto(RemoteKey key) {
    switch (key) {
        case RemoteKey::Power:
        case RemoteKey::PowerOn:
        case RemoteKey::PowerOff:
        case RemoteKey::EzAdjust:
        case RemoteKey::InStart:
            return true;
        default:
            return false;
    }
}

}  // namespace

CommandRouter::CommandRouter(IrTransport& ir, WebOsClient& lan, IpControlClient& ipControl, BleHidTransport& ble)
    : ir_(ir), lan_(lan), ipControl_(ipControl), ble_(ble) {}

void CommandRouter::setMode(LinkMode mode) {
    mode_ = mode;
    // Two TLS sessions plus the BLE stack do not fit in RAM together (no PSRAM),
    // so the network clients pause while Bluetooth is the active link.
    const bool bluetooth = mode_ == LinkMode::Ble;
    lan_.setSuspended(bluetooth);
    ipControl_.setSuspended(bluetooth);
    if (bluetooth) {
        ble_.begin();
    } else {
        ble_.end();
    }
}

Transport& CommandRouter::primary() const {
    switch (mode_) {
        case LinkMode::Ir: return ir_;
        case LinkMode::Ble: return ble_;
        case LinkMode::Lan:
            if (!lan_.isReady() && ipControl_.isReady()) {
                return ipControl_;
            }
            return lan_;
        case LinkMode::Auto:
        default:
            if (lan_.isReady()) {
                return lan_;
            }
            if (ipControl_.isReady()) {
                return ipControl_;
            }
            return ir_;
    }
}

int CommandRouter::candidatesFor(RemoteKey key, Transport* out[kMaxCandidates]) const {
    switch (mode_) {
        case LinkMode::Ir:
            out[0] = &ir_;
            return 1;
        case LinkMode::Ble:
            out[0] = &ble_;
            out[1] = &ir_;
            return 2;
        case LinkMode::Lan:
        case LinkMode::Auto:
        default:
            if (mode_ == LinkMode::Auto && isIrPreferredInAuto(key)) {
                out[0] = &ir_;
                return 1;
            }
            out[0] = &lan_;
            out[1] = &ipControl_;
            out[2] = &ir_;
            return 3;
    }
}

Transport* CommandRouter::sendKey(RemoteKey key) {
    if (key == RemoteKey::Power || key == RemoteKey::PowerOn) {
        return sendPower(key);
    }
    Transport* candidates[kMaxCandidates];
    const int count = candidatesFor(key, candidates);
    for (int i = 0; i < count; ++i) {
        Transport* transport = candidates[i];
        if (transport->isReady() && transport->supports(key) && transport->sendKey(key)) {
            return transport;
        }
    }
    return nullptr;
}

Transport* CommandRouter::sendPower(RemoteKey key) {
    if (mode_ == LinkMode::Ir || mode_ == LinkMode::Auto) {
        // IR power works with the TV off; Wake-on-LAN helps when the remote is not aimed.
        if (!lan_.isReady() && !wakeMac_.isEmpty()) {
            sendWakeOnLan(wakeMac_);
        }
        ir_.sendKey(key);
        return &ir_;
    }
    // LAN / BLE: turn off over the link when connected, otherwise wake the TV.
    // A discrete IR "power on" is sent as well because Wake-on-LAN only works when
    // "Turn on via Wi-Fi" is enabled on the TV.
    if (key == RemoteKey::Power) {
        Transport* candidates[kMaxCandidates];
        const int count = candidatesFor(key, candidates);
        for (int i = 0; i < count; ++i) {
            Transport* transport = candidates[i];
            if (transport != &ir_ && transport->isReady() && transport->supports(key) && transport->sendKey(key)) {
                return transport;
            }
        }
    }
    if (!wakeMac_.isEmpty()) {
        sendWakeOnLan(wakeMac_);
    }
    ir_.sendKey(RemoteKey::PowerOn);
    return &ir_;
}

Transport* CommandRouter::pointerTransport() const {
    switch (mode_) {
        case LinkMode::Ble: return ble_.isReady() ? &ble_ : nullptr;
        case LinkMode::Ir: return nullptr;
        case LinkMode::Lan:
        case LinkMode::Auto:
        default: return lan_.isReady() ? &lan_ : nullptr;
    }
}

Transport* CommandRouter::textTransport() const {
    return pointerTransport();
}

const char* CommandRouter::pointerUnavailableReason() const {
    switch (mode_) {
        case LinkMode::Ir: return str::kNeedsLanOrBle;
        case LinkMode::Ble: return str::kBleNotConnected;
        default: return lan_.status() == WebOsStatus::NotConfigured ? str::kSetUpTvFirst : str::kTvNotConnected;
    }
}

}  // namespace lgremote
