// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

#include "app/Settings.h"
#include "core/Transport.h"

namespace lgremote {

class IrTransport;
class WebOsClient;
class IpControlClient;
class BleHidTransport;

// Decides which transport delivers each command, based on the selected link mode.
// The network ("LAN") link prefers the webOS API and falls back to LG IP Control;
// infrared is the universal fallback: it works with every LG TV, even when it is off.
class CommandRouter {
public:
    CommandRouter(IrTransport& ir, WebOsClient& lan, IpControlClient& ipControl, BleHidTransport& ble);

    void setMode(LinkMode mode);
    LinkMode mode() const { return mode_; }

    // Set by the application; used for Wake-on-LAN when powering on over the network.
    void setWakeMac(const String& mac) { wakeMac_ = mac; }

    // Sends the key; returns the transport that handled it or nullptr.
    Transport* sendKey(RemoteKey key);

    // The transport that the status bar describes for the current mode.
    Transport& primary() const;

    // Transport able to move a cursor / type text right now, or nullptr.
    Transport* pointerTransport() const;
    Transport* textTransport() const;

    // Why pointer/text is not available (for user feedback).
    const char* pointerUnavailableReason() const;

private:
    static constexpr int kMaxCandidates = 3;

    int candidatesFor(RemoteKey key, Transport* out[kMaxCandidates]) const;
    Transport* sendPower(RemoteKey key);

    IrTransport& ir_;
    WebOsClient& lan_;
    IpControlClient& ipControl_;
    BleHidTransport& ble_;
    LinkMode mode_ = LinkMode::Auto;
    String wakeMac_;
};

}  // namespace lgremote
