// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

#include <atomic>

#include "core/Transport.h"

namespace lgremote {

// USB HID keyboard + mouse + consumer control over the Cardputer's USB-C port.
// LG Smart TVs accept USB keyboards and mice since 2012, including webOS versions
// that have no Bluetooth keyboard/mouse pairing, so this is the cable alternative to BLE.
// USB is started lazily: until then the Cardputer only charges when plugged in.
class UsbHidTransport : public Transport {
public:
    void begin();
    bool started() const { return started_; }
    bool isMounted() const { return mounted_.load(); }

    const char* name() const override { return "USB"; }
    LinkState state() const override;
    bool supports(RemoteKey key) const override;
    bool sendKey(RemoteKey key) override;

    bool supportsPointer() const override { return true; }
    void movePointer(int dx, int dy) override;
    void click() override;
    void scroll(int dy) override;

    bool supportsText() const override { return true; }
    void typeText(const char* text) override;
    void deleteText(int count) override;
    void sendEnter() override;

    // Called from the USB event handler.
    void setMounted(bool mounted) { mounted_ = mounted; }

private:
    void tapKey(uint8_t usage);
    void tapConsumer(uint16_t usage);

    bool started_ = false;
    std::atomic<bool> mounted_{false};
};

}  // namespace lgremote
