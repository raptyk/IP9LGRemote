// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#include "WebOsClient.h"

#include <ArduinoJson.h>
#include <WiFi.h>
#include <lwip/etharp.h>
#include <lwip/netif.h>

#include "SsapManifest.h"
#include "WebOsKeys.h"

namespace lgremote {

namespace {

constexpr uint16_t kTlsPort = 3001;    // webOS 4+ (2018 and newer), self-signed certificate
constexpr uint16_t kPlainPort = 3000;  // legacy webOS 1-3 TVs
constexpr uint32_t kTaskStackBytes = 12 * 1024;
constexpr uint32_t kConnectTimeoutMs = 8000;
constexpr uint32_t kPointerFlushIntervalMs = 20;
constexpr uint32_t kPointerRetryMs = 5000;
constexpr uint32_t kRetryDelaysMs[] = {2000, 5000, 10000, 20000, 30000};

class MutexLock {
public:
    explicit MutexLock(SemaphoreHandle_t mutex) : mutex_(mutex) { xSemaphoreTake(mutex_, portMAX_DELAY); }
    ~MutexLock() { xSemaphoreGive(mutex_); }
    MutexLock(const MutexLock&) = delete;
    MutexLock& operator=(const MutexLock&) = delete;

private:
    SemaphoreHandle_t mutex_;
};

void copyText(char* destination, size_t size, const char* source) {
    if (source == nullptr) {
        destination[0] = '\0';
        return;
    }
    strlcpy(destination, source, size);
}

}  // namespace

const char* webOsStatusText(WebOsStatus status) {
    switch (status) {
        case WebOsStatus::NotConfigured: return "No TV set";
        case WebOsStatus::NoNetwork: return "No Wi-Fi";
        case WebOsStatus::Connecting: return "Connecting";
        case WebOsStatus::WaitingForUser: return "Accept on TV";
        case WebOsStatus::Ready: return "Connected";
        case WebOsStatus::Rejected: return "Pairing declined";
        case WebOsStatus::Unreachable: return "TV unreachable";
    }
    return "";
}

WebOsClient::WebOsClient() {
    mutex_ = xSemaphoreCreateMutex();
    queue_ = xQueueCreate(24, sizeof(Command));
}

WebOsClient::~WebOsClient() {
    if (task_ != nullptr) {
        vTaskDelete(task_);
    }
    vQueueDelete(queue_);
    vSemaphoreDelete(mutex_);
}

void WebOsClient::begin() {
    mainSocket_.onEvent([this](WStype_t type, uint8_t* payload, size_t length) {
        handleMainEvent(type, payload, length);
    });
    pointerSocket_.onEvent([this](WStype_t type, uint8_t* payload, size_t length) {
        handlePointerEvent(type, payload, length);
    });
    // Arduino loop() runs on core 1; networking lives on core 0 next to the Wi-Fi stack.
    xTaskCreatePinnedToCore(taskEntry, "webos", kTaskStackBytes, this, 2, &task_, 0);
}

void WebOsClient::configure(const String& host, const String& clientKey) {
    MutexLock lock(mutex_);
    host_ = host;
    clientKey_ = clientKey;
    configChanged_ = true;
}

bool WebOsClient::takeNewClientKey(String& key) {
    MutexLock lock(mutex_);
    if (!hasNewClientKey_) {
        return false;
    }
    key = newClientKey_;
    hasNewClientKey_ = false;
    return true;
}

bool WebOsClient::takeLearnedMac(String& mac) {
    MutexLock lock(mutex_);
    if (!hasLearnedMac_) {
        return false;
    }
    mac = learnedMac_;
    hasLearnedMac_ = false;
    return true;
}

LinkState WebOsClient::state() const {
    switch (status_.load()) {
        case WebOsStatus::Ready: return LinkState::Ready;
        case WebOsStatus::Connecting:
        case WebOsStatus::WaitingForUser: return LinkState::Connecting;
        case WebOsStatus::NotConfigured: return LinkState::Off;
        default: return LinkState::Error;
    }
}

bool WebOsClient::supports(RemoteKey key) const {
    return key == RemoteKey::PlayPause || webos::findKeyMapping(key) != nullptr;
}

bool WebOsClient::sendKey(RemoteKey key) {
    if (!isReady() || !supports(key)) {
        return false;
    }
    enqueue(CommandType::Key, nullptr, static_cast<int16_t>(key));
    return true;
}

void WebOsClient::movePointer(int dx, int dy) {
    pendingDx_ += dx;
    pendingDy_ += dy;
}

void WebOsClient::click() { enqueue(CommandType::Click); }
void WebOsClient::scroll(int dy) { enqueue(CommandType::Scroll, nullptr, static_cast<int16_t>(dy)); }
void WebOsClient::typeText(const char* text) { enqueue(CommandType::InsertText, text); }
void WebOsClient::deleteText(int count) { enqueue(CommandType::DeleteText, nullptr, static_cast<int16_t>(count)); }
void WebOsClient::sendEnter() { enqueue(CommandType::Enter); }
void WebOsClient::launchApp(const char* appId) { enqueue(CommandType::Launch, appId); }
void WebOsClient::showToast(const char* message) { enqueue(CommandType::Toast, message); }
void WebOsClient::turnOff() { enqueue(CommandType::TurnOff); }

void WebOsClient::enqueue(CommandType type, const char* text, int16_t value) {
    Command command{};
    command.type = type;
    command.value = value;
    copyText(command.text, sizeof(command.text), text);
    if (xQueueSend(queue_, &command, 0) != pdTRUE) {
        log_w("webOS command queue full, dropping command %d", static_cast<int>(type));
    }
}

void WebOsClient::taskEntry(void* arg) {
    static_cast<WebOsClient*>(arg)->taskLoop();
}

void WebOsClient::taskLoop() {
    for (;;) {
        bool reconfigure = false;
        {
            MutexLock lock(mutex_);
            if (configChanged_) {
                configChanged_ = false;
                reconfigure = true;
                activeHost_ = host_;
                activeKey_ = clientKey_;
            }
        }
        if (reconfigure) {
            stopConnections();
            useTls_ = true;
            failedAttempts_ = 0;
            macLearned_ = false;
            nextAttemptAt_ = 0;
            status_ = WebOsStatus::Connecting;
        }

        const uint32_t now = millis();
        if (activeHost_.isEmpty()) {
            status_ = WebOsStatus::NotConfigured;
        } else if (!networkAvailable_ || suspended_) {
            if (mainConnected_ || connectStartedAt_ != 0) {
                stopConnections();
            }
            status_ = WebOsStatus::NoNetwork;
        } else if (!mainConnected_ && connectStartedAt_ == 0 && now >= nextAttemptAt_ &&
                   status_ != WebOsStatus::Rejected) {
            startMainConnection();
        }

        if (connectStartedAt_ != 0 || mainConnected_) {
            mainSocket_.loop();
        }
        if (pointerStarted_) {
            pointerSocket_.loop();
        }

        // Give up on a connection attempt that hangs (TV in deep standby, wrong IP).
        if (!mainConnected_ && connectStartedAt_ != 0 && millis() - connectStartedAt_ > kConnectTimeoutMs) {
            mainSocket_.disconnect();
            connectStartedAt_ = 0;
            ++failedAttempts_;
            // A TV that never answers on 3001 may be an old model: alternate with plain 3000.
            if (failedAttempts_ >= 2) {
                useTls_ = !useTls_;
            }
            const size_t index = min<size_t>(failedAttempts_, sizeof(kRetryDelaysMs) / sizeof(kRetryDelaysMs[0])) - 1;
            nextAttemptAt_ = millis() + kRetryDelaysMs[index];
            status_ = WebOsStatus::Unreachable;
        }

        Command command;
        while (xQueueReceive(queue_, &command, 0) == pdTRUE) {
            processCommand(command);
        }
        if (millis() - lastPointerFlushAt_ >= kPointerFlushIntervalMs) {
            flushPointerMotion();
            lastPointerFlushAt_ = millis();
        }

        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void WebOsClient::startMainConnection() {
    status_ = WebOsStatus::Connecting;
    registered_ = false;
    connectStartedAt_ = millis();
    // An empty protocol string suppresses the Sec-WebSocket-Protocol header, which webOS does not expect.
    if (useTls_) {
        mainSocket_.beginSSL(activeHost_.c_str(), kTlsPort, "/", "", "");
    } else {
        mainSocket_.begin(activeHost_.c_str(), kPlainPort, "/", "");
    }
    mainSocket_.setReconnectInterval(0x7FFFFFFF);  // reconnection is driven by this class
}

void WebOsClient::stopConnections() {
    pointerSocket_.disconnect();
    mainSocket_.disconnect();
    mainConnected_ = false;
    pointerConnected_ = false;
    pointerStarted_ = false;
    registered_ = false;
    connectStartedAt_ = 0;
}

void WebOsClient::handleMainEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_CONNECTED:
            log_i("webOS connected (%s)", useTls_ ? "wss:3001" : "ws:3000");
            mainConnected_ = true;
            connectStartedAt_ = 0;
            failedAttempts_ = 0;
            // Since webOS firmware 33.20 (2025) the TV only shows the pairing prompt after a
            // "hello" with an (empty) payload, and getSystemInfo must precede "register".
            mainSocket_.sendTXT("{\"id\":\"hello\",\"type\":\"hello\",\"payload\":{}}");
            sendRequest("ssap://system/getSystemInfo", "{}", "sys");
            sendRegister();
            break;
        case WStype_DISCONNECTED:
            if (mainConnected_) {
                log_i("webOS disconnected");
                nextAttemptAt_ = millis() + kRetryDelaysMs[0];
            }
            mainConnected_ = false;
            registered_ = false;
            if (status_ == WebOsStatus::Ready || status_ == WebOsStatus::WaitingForUser) {
                status_ = WebOsStatus::Unreachable;
            }
            pointerSocket_.disconnect();
            pointerConnected_ = false;
            pointerStarted_ = false;
            break;
        case WStype_TEXT:
            handleMessage(reinterpret_cast<const char*>(payload), length);
            break;
        default:
            break;
    }
}

void WebOsClient::handlePointerEvent(WStype_t type, uint8_t* /*payload*/, size_t /*length*/) {
    if (type == WStype_CONNECTED) {
        pointerConnected_ = true;
        log_i("webOS pointer socket connected");
    } else if (type == WStype_DISCONNECTED) {
        pointerConnected_ = false;
        // The pointer socket expires after inactivity on some firmware; request a fresh one,
        // but not more often than every few seconds if the TV keeps refusing it.
        if (registered_ && millis() - lastPointerRequestAt_ > kPointerRetryMs) {
            lastPointerRequestAt_ = millis();
            sendRequest("ssap://com.webos.service.networkinput/getPointerInputSocket", nullptr, "ptr");
        }
    }
}

void WebOsClient::handleMessage(const char* json, size_t length) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json, length);
    if (error) {
        log_w("webOS: invalid JSON (%s)", error.c_str());
        return;
    }

    const char* type = doc["type"] | "";
    const char* id = doc["id"] | "";
    JsonVariantConst payload = doc["payload"];

    if (strcmp(type, "registered") == 0) {
        const char* key = payload["client-key"] | "";
        if (strlen(key) > 0 && activeKey_ != key) {
            activeKey_ = key;
            MutexLock lock(mutex_);
            clientKey_ = key;
            newClientKey_ = key;
            hasNewClientKey_ = true;
        }
        registered_ = true;
        status_ = WebOsStatus::Ready;
        sendRequest("ssap://com.webos.service.networkinput/getPointerInputSocket", nullptr, "ptr");
        sendRequest("ssap://audio/getVolume", nullptr, "vol");
        sendRequest("ssap://com.webos.service.ime/registerRemoteKeyboard", nullptr, "kbd", "subscribe");
        learnMacAddress();
        return;
    }

    if (strncmp(id, "reg", 3) == 0) {
        if (strcmp(type, "error") == 0) {
            log_w("webOS registration failed: %s", doc["error"] | "");
            status_ = WebOsStatus::Rejected;
            mainSocket_.disconnect();
        } else if (strcmp(payload["pairingType"] | "", "PROMPT") == 0) {
            status_ = WebOsStatus::WaitingForUser;
        }
        return;
    }

    if (strcmp(type, "error") == 0) {
        // getSystemInfo may fail with 401 depending on firmware; that is expected.
        log_w("webOS request %s failed: %s", id, doc["error"] | "");
        return;
    }

    if (strncmp(id, "ptr", 3) == 0) {
        const char* socketPath = payload["socketPath"] | "";
        if (strlen(socketPath) > 0) {
            connectPointerSocket(socketPath);
        }
        return;
    }

    if (strncmp(id, "kbd", 3) == 0) {
        // Tells whether a text field on the TV has focus (on-screen keyboard is open).
        JsonVariantConst widget = payload["currentWidget"];
        if (!widget.isNull()) {
            textFieldFocused_ = widget["focus"] | false;
        }
        return;
    }

    if (strncmp(id, "vol", 3) == 0) {
        // webOS 5+ nests the values in "volumeStatus", older versions put them at the top level.
        JsonVariantConst status = payload["volumeStatus"];
        int level = status.isNull() ? (payload["volume"] | -1) : (status["volume"] | -1);
        if (level >= 0) {
            volume_ = level;
            volumeChangedAt_ = millis();
        }
    }
}

void WebOsClient::sendRegister() {
    JsonDocument doc;
    doc["type"] = "register";
    doc["id"] = "reg_0";
    JsonObject payload = doc["payload"].to<JsonObject>();
    payload["forcePairing"] = false;
    payload["pairingType"] = "PROMPT";
    if (!activeKey_.isEmpty()) {
        payload["client-key"] = activeKey_;
    }
    JsonDocument manifest;
    deserializeJson(manifest, webos::kManifestJson);
    payload["manifest"] = manifest;

    String message;
    serializeJson(doc, message);
    mainSocket_.sendTXT(message);
}

void WebOsClient::sendRequest(const char* uri, const char* payloadJson, const char* idPrefix, const char* type) {
    if (!mainConnected_) {
        return;
    }
    char id[24];
    snprintf(id, sizeof(id), "%s_%lu", idPrefix, static_cast<unsigned long>(++requestCounter_));

    String message;
    message.reserve(96 + (payloadJson ? strlen(payloadJson) : 0));
    message += "{\"type\":\"";
    message += type;
    message += "\",\"id\":\"";
    message += id;
    message += "\",\"uri\":\"";
    message += uri;
    message += "\"";
    if (payloadJson != nullptr) {
        message += ",\"payload\":";
        message += payloadJson;
    }
    message += "}";
    mainSocket_.sendTXT(message);
}

void WebOsClient::connectPointerSocket(const String& socketPath) {
    // socketPath looks like "wss://192.168.1.20:3001/resources/<id>/netinput.pointer.sock".
    const bool tls = socketPath.startsWith("wss://");
    const int hostStart = socketPath.indexOf("://") + 3;
    const int pathStart = socketPath.indexOf('/', hostStart);
    if (hostStart < 3 || pathStart < 0) {
        log_w("webOS: unexpected pointer socket path %s", socketPath.c_str());
        return;
    }
    String hostPort = socketPath.substring(hostStart, pathStart);
    String path = socketPath.substring(pathStart);
    String host = hostPort;
    uint16_t port = tls ? kTlsPort : kPlainPort;
    const int colon = hostPort.indexOf(':');
    if (colon >= 0) {
        host = hostPort.substring(0, colon);
        port = static_cast<uint16_t>(hostPort.substring(colon + 1).toInt());
    }

    pointerSocket_.disconnect();
    if (tls) {
        pointerSocket_.beginSSL(host.c_str(), port, path.c_str(), "", "");
    } else {
        pointerSocket_.begin(host.c_str(), port, path.c_str(), "");
    }
    pointerSocket_.setReconnectInterval(0x7FFFFFFF);
    pointerStarted_ = true;
}

void WebOsClient::sendPointer(const String& message) {
    if (pointerConnected_) {
        String copy = message;
        pointerSocket_.sendTXT(copy);
    }
}

void WebOsClient::processCommand(const Command& command) {
    if (!registered_) {
        return;
    }
    switch (command.type) {
        case CommandType::Key: {
            RemoteKey key = static_cast<RemoteKey>(command.value);
            if (key == RemoteKey::PlayPause) {
                key = playing_ ? RemoteKey::Pause : RemoteKey::Play;
                playing_ = !playing_;
            }
            const webos::KeyMapping* mapping = webos::findKeyMapping(key);
            if (mapping == nullptr) {
                break;
            }
            if (mapping->button != nullptr) {
                sendPointer(String("type:button\nname:") + mapping->button + "\n\n");
            } else {
                sendRequest(mapping->uri, mapping->payload);
            }
            if (key == RemoteKey::VolumeUp || key == RemoteKey::VolumeDown || key == RemoteKey::Mute) {
                sendRequest("ssap://audio/getVolume", nullptr, "vol");
            }
            break;
        }
        case CommandType::Click:
            flushPointerMotion();
            sendPointer("type:click\n\n");
            break;
        case CommandType::Scroll:
            sendPointer(String("type:scroll\ndx:0\ndy:") + command.value + "\n\n");
            break;
        case CommandType::InsertText: {
            JsonDocument payload;
            payload["text"] = command.text;
            payload["replace"] = 0;
            String json;
            serializeJson(payload, json);
            sendRequest("ssap://com.webos.service.ime/insertText", json.c_str());
            break;
        }
        case CommandType::DeleteText: {
            char json[24];
            snprintf(json, sizeof(json), "{\"count\":%d}", command.value);
            sendRequest("ssap://com.webos.service.ime/deleteCharacters", json);
            break;
        }
        case CommandType::Enter:
            sendRequest("ssap://com.webos.service.ime/sendEnterKey");
            break;
        case CommandType::Launch: {
            JsonDocument payload;
            payload["id"] = command.text;
            String json;
            serializeJson(payload, json);
            sendRequest("ssap://system.launcher/launch", json.c_str());
            break;
        }
        case CommandType::Toast: {
            JsonDocument payload;
            payload["message"] = command.text;
            String json;
            serializeJson(payload, json);
            sendRequest("ssap://system.notifications/createToast", json.c_str());
            break;
        }
        case CommandType::TurnOff:
            sendRequest("ssap://system/turnOff");
            break;
    }
}

void WebOsClient::flushPointerMotion() {
    const int dx = pendingDx_.exchange(0);
    const int dy = pendingDy_.exchange(0);
    if ((dx != 0 || dy != 0) && pointerConnected_) {
        char message[48];
        snprintf(message, sizeof(message), "type:move\ndx:%d\ndy:%d\ndown:0\n\n", dx, dy);
        pointerSocket_.sendTXT(message);
    }
}

void WebOsClient::learnMacAddress() {
    if (macLearned_) {
        return;
    }
    IPAddress address;
    if (!address.fromString(activeHost_)) {
        return;
    }
    ip4_addr_t target;
    IP4_ADDR(&target, address[0], address[1], address[2], address[3]);
    struct eth_addr* mac = nullptr;
    const ip4_addr_t* entryIp = nullptr;
    if (netif_default == nullptr || etharp_find_addr(netif_default, &target, &mac, &entryIp) < 0 || mac == nullptr) {
        return;
    }
    char text[18];
    snprintf(text, sizeof(text), "%02X:%02X:%02X:%02X:%02X:%02X", mac->addr[0], mac->addr[1], mac->addr[2],
             mac->addr[3], mac->addr[4], mac->addr[5]);
    macLearned_ = true;
    MutexLock lock(mutex_);
    learnedMac_ = text;
    hasLearnedMac_ = true;
}

}  // namespace lgremote
