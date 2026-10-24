// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

#include <Arduino.h>
#include <WebSocketsClient.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

#include <atomic>

#include "core/Transport.h"

namespace lgremote {

enum class WebOsStatus : uint8_t {
    NotConfigured,   // no TV address
    NoNetwork,       // Wi-Fi not connected
    Connecting,      // TCP/TLS/WebSocket handshake
    WaitingForUser,  // pairing prompt is shown on the TV
    Ready,
    Rejected,        // user declined the pairing prompt
    Unreachable      // TV is off or not answering
};

const char* webOsStatusText(WebOsStatus status);

// Client for the LG webOS "second screen" API (SSAP over WebSocket, the protocol used
// by the LG ThinQ app). It runs in its own FreeRTOS task so that slow TLS handshakes
// never freeze the user interface; the public methods only enqueue work.
class WebOsClient : public Transport {
public:
    WebOsClient();
    ~WebOsClient() override;

    void begin();

    // Thread-safe: may be called at any time from the UI task.
    void configure(const String& host, const String& clientKey);
    void setNetworkAvailable(bool available) { networkAvailable_ = available; }
    WebOsStatus status() const { return status_.load(); }

    // Returns true once after the TV issued a new pairing key (persist it in settings).
    bool takeNewClientKey(String& key);
    // Returns true once after the TV MAC address was learned (for Wake-on-LAN).
    bool takeLearnedMac(String& mac);
    // Last known volume (0..100) or -1.
    int volume() const { return volume_.load(); }
    // True while a text field on the TV has focus (its on-screen keyboard is open).
    bool textFieldFocused() const { return textFieldFocused_.load(); }
    // Drops both connections, e.g. to free TLS memory while Bluetooth is in use.
    void setSuspended(bool suspended) { suspended_ = suspended; }
    uint32_t volumeChangedAt() const { return volumeChangedAt_.load(); }

    void launchApp(const char* appId);
    void showToast(const char* message);
    void turnOff();

    // Transport
    const char* name() const override { return "LAN"; }
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

private:
    enum class CommandType : uint8_t { Key, Click, Scroll, InsertText, DeleteText, Enter, Launch, Toast, TurnOff };

    struct Command {
        CommandType type;
        int16_t value;
        char text[64];
    };

    static void taskEntry(void* arg);
    void taskLoop();

    void enqueue(CommandType type, const char* text = nullptr, int16_t value = 0);
    void startMainConnection();
    void stopConnections();
    void handleMainEvent(WStype_t type, uint8_t* payload, size_t length);
    void handlePointerEvent(WStype_t type, uint8_t* payload, size_t length);
    void handleMessage(const char* json, size_t length);
    void sendRegister();
    // `idPrefix` tells the response handler what the request was for ("cmd", "ptr", "vol", "kbd").
    void sendRequest(const char* uri, const char* payloadJson = nullptr, const char* idPrefix = "cmd",
                     const char* type = "request");
    void connectPointerSocket(const String& socketPath);
    void sendPointer(const String& message);
    void processCommand(const Command& command);
    void flushPointerMotion();
    void learnMacAddress();

    WebSocketsClient mainSocket_;
    WebSocketsClient pointerSocket_;
    TaskHandle_t task_ = nullptr;
    QueueHandle_t queue_ = nullptr;
    SemaphoreHandle_t mutex_ = nullptr;

    // Shared with the UI task (guarded by mutex_).
    String host_;
    String clientKey_;
    String newClientKey_;
    String learnedMac_;
    bool configChanged_ = false;
    bool hasNewClientKey_ = false;
    bool hasLearnedMac_ = false;

    std::atomic<WebOsStatus> status_{WebOsStatus::NotConfigured};
    std::atomic<bool> networkAvailable_{false};
    std::atomic<int> pendingDx_{0};
    std::atomic<int> pendingDy_{0};
    std::atomic<int> volume_{-1};
    std::atomic<bool> textFieldFocused_{false};
    std::atomic<bool> suspended_{false};
    std::atomic<uint32_t> volumeChangedAt_{0};

    // Network task only.
    String activeHost_;
    String activeKey_;
    bool useTls_ = true;
    bool mainConnected_ = false;
    bool pointerConnected_ = false;
    bool pointerStarted_ = false;  // pointerSocket_ has a target (begin/beginSSL was called)
    uint32_t lastPointerRequestAt_ = 0;
    bool registered_ = false;
    bool macLearned_ = false;
    uint8_t failedAttempts_ = 0;
    uint32_t nextAttemptAt_ = 0;
    uint32_t connectStartedAt_ = 0;
    uint32_t lastPointerFlushAt_ = 0;
    uint32_t requestCounter_ = 0;
    bool playing_ = false;
};

}  // namespace lgremote
