// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

#include <Arduino.h>
#include <WiFiClient.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

#include <atomic>

#include "core/Transport.h"

namespace lgremote {

// LG "IP Control" (TCP port 9761): text commands such as "KEY_ACTION volumeup",
// encrypted with AES-128 using a key derived from the 8-character keycode shown in
// Settings > Support > IP Control Settings (webOS 23+). No TLS, no pairing prompt and
// not affected by the SSAP permission lockdown, but no pointer or text input.
// Protocol reference: WesSouza/lgtv-ip-control.
class IpControlClient : public Transport {
public:
    IpControlClient();
    ~IpControlClient() override;

    void begin();
    void configure(const String& host, const String& keycode);
    void setNetworkAvailable(bool available) { networkAvailable_ = available; }
    void setSuspended(bool suspended) { suspended_ = suspended; }
    bool isConfigured() const { return configured_.load(); }

    static bool isValidKeycode(const String& keycode);

    const char* name() const override { return "IP"; }
    LinkState state() const override;
    bool supports(RemoteKey key) const override;
    bool sendKey(RemoteKey key) override;

    void launchApp(const char* appId);

private:
    struct Command {
        char text[48];
    };

    static void taskEntry(void* arg);
    void taskLoop();
    void enqueue(const String& command);
    bool ensureConnected();
    bool deriveKey(const String& keycode);
    bool sendEncrypted(const char* command);
    void drainResponse();

    WiFiClient client_;
    TaskHandle_t task_ = nullptr;
    QueueHandle_t queue_ = nullptr;
    SemaphoreHandle_t mutex_ = nullptr;

    String host_;     // guarded by mutex_
    String keycode_;  // guarded by mutex_
    bool configChanged_ = false;

    std::atomic<bool> configured_{false};
    std::atomic<bool> connected_{false};
    std::atomic<bool> networkAvailable_{false};
    std::atomic<bool> suspended_{false};

    // Network task only.
    String activeHost_;
    uint8_t key_[16] = {};
    bool keyReady_ = false;
    uint32_t nextConnectAt_ = 0;
};

}  // namespace lgremote
