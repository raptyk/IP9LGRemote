// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>

#include <atomic>

#include "core/Transport.h"

namespace lgremote {

// Bluetooth LE HID keyboard + mouse + consumer control.
// webOS TVs with Bluetooth accept it via Settings > General > External Devices >
// Bluetooth Controller (or the "Connect Bluetooth controller" menu on older models).
class BleHidTransport : public Transport, private NimBLEServerCallbacks {
public:
    explicit BleHidTransport(const char* deviceName);

    // The BLE stack is started lazily the first time it is needed to save RAM.
    void begin();
    // Stops the BLE stack and returns its RAM to the heap (bonds stay in NVS).
    void end();
    bool started() const { return started_; }

    // Diagnostics for the key test screen: webOS support for HID usages varies by model.
    void sendRawKeyboard(uint8_t usage, uint8_t modifiers = 0) { tapKey(usage, modifiers); }
    void sendRawConsumer(uint16_t usage) { tapConsumer(usage); }

    void startPairing();
    void forgetBonds();
    bool isConnected() const { return connected_.load(); }
    bool isAdvertising() const;

    const char* name() const override { return "BLE"; }
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

    void loop() override;

private:
    void onConnect(NimBLEServer* server, NimBLEConnInfo& connInfo) override;
    void onDisconnect(NimBLEServer* server, NimBLEConnInfo& connInfo, int reason) override;

    void tapKey(uint8_t usage, uint8_t modifiers = 0);
    void tapConsumer(uint16_t usage);
    void sendMouse(uint8_t buttons, int dx, int dy, int wheel);

    const char* deviceName_;
    bool started_ = false;
    std::atomic<bool> connected_{false};
    NimBLEServer* server_ = nullptr;
    NimBLEHIDDevice* hid_ = nullptr;
    NimBLECharacteristic* keyboardInput_ = nullptr;
    NimBLECharacteristic* mouseInput_ = nullptr;
    NimBLECharacteristic* consumerInput_ = nullptr;
    uint32_t restartAdvertisingAt_ = 0;
};

}  // namespace lgremote
