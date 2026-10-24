// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#include "IpControlClient.h"

#include <esp_random.h>
#include <mbedtls/aes.h>
#include <mbedtls/md.h>
#include <mbedtls/pkcs5.h>

namespace lgremote {

namespace {

constexpr uint16_t kPort = 9761;
constexpr uint32_t kConnectTimeoutMs = 1500;
constexpr uint32_t kReconnectIntervalMs = 5000;
constexpr uint32_t kResponseTimeoutMs = 600;
constexpr uint32_t kPbkdf2Iterations = 16384;
constexpr size_t kBlockSize = 16;
constexpr uint32_t kTaskStackBytes = 6 * 1024;

constexpr uint8_t kSalt[16] = {0x63, 0x61, 0xb8, 0x0e, 0x9b, 0xdc, 0xa6, 0x63,
                               0x8d, 0x07, 0x20, 0xf2, 0xcc, 0x56, 0x8f, 0xb9};

struct IpKey {
    RemoteKey key;
    const char* command;
};

constexpr IpKey kIpKeys[] = {
    {RemoteKey::Up, "KEY_ACTION arrowup"},
    {RemoteKey::Down, "KEY_ACTION arrowdown"},
    {RemoteKey::Left, "KEY_ACTION arrowleft"},
    {RemoteKey::Right, "KEY_ACTION arrowright"},
    {RemoteKey::Ok, "KEY_ACTION ok"},
    {RemoteKey::Back, "KEY_ACTION returnback"},
    {RemoteKey::Home, "KEY_ACTION myapp"},
    {RemoteKey::Settings, "KEY_ACTION settingmenu"},
    {RemoteKey::Info, "KEY_ACTION programminfo"},
    {RemoteKey::Guide, "KEY_ACTION programguide"},
    {RemoteKey::Input, "KEY_ACTION deviceinput"},
    {RemoteKey::VolumeUp, "KEY_ACTION volumeup"},
    {RemoteKey::VolumeDown, "KEY_ACTION volumedown"},
    {RemoteKey::Mute, "KEY_ACTION volumemute"},
    {RemoteKey::ChannelUp, "KEY_ACTION channelup"},
    {RemoteKey::ChannelDown, "KEY_ACTION channeldown"},
    {RemoteKey::Digit0, "KEY_ACTION number0"},
    {RemoteKey::Digit1, "KEY_ACTION number1"},
    {RemoteKey::Digit2, "KEY_ACTION number2"},
    {RemoteKey::Digit3, "KEY_ACTION number3"},
    {RemoteKey::Digit4, "KEY_ACTION number4"},
    {RemoteKey::Digit5, "KEY_ACTION number5"},
    {RemoteKey::Digit6, "KEY_ACTION number6"},
    {RemoteKey::Digit7, "KEY_ACTION number7"},
    {RemoteKey::Digit8, "KEY_ACTION number8"},
    {RemoteKey::Digit9, "KEY_ACTION number9"},
    {RemoteKey::Play, "KEY_ACTION play"},
    {RemoteKey::Rewind, "KEY_ACTION rewind"},
    {RemoteKey::FastForward, "KEY_ACTION fastforward"},
    {RemoteKey::Red, "KEY_ACTION redbutton"},
    {RemoteKey::Green, "KEY_ACTION greenbutton"},
    {RemoteKey::Yellow, "KEY_ACTION yellowbutton"},
    {RemoteKey::Blue, "KEY_ACTION bluebutton"},
    {RemoteKey::Power, "POWER off"},
    {RemoteKey::PowerOff, "POWER off"},
    {RemoteKey::Hdmi1, "INPUT_SELECT hdmi1"},
    {RemoteKey::Hdmi2, "INPUT_SELECT hdmi2"},
    {RemoteKey::Hdmi3, "INPUT_SELECT hdmi3"},
    {RemoteKey::Hdmi4, "INPUT_SELECT hdmi4"},
};

const char* findCommand(RemoteKey key) {
    for (const IpKey& entry : kIpKeys) {
        if (entry.key == key) {
            return entry.command;
        }
    }
    return nullptr;
}

class MutexLock {
public:
    explicit MutexLock(SemaphoreHandle_t mutex) : mutex_(mutex) { xSemaphoreTake(mutex_, portMAX_DELAY); }
    ~MutexLock() { xSemaphoreGive(mutex_); }
    MutexLock(const MutexLock&) = delete;
    MutexLock& operator=(const MutexLock&) = delete;

private:
    SemaphoreHandle_t mutex_;
};

}  // namespace

IpControlClient::IpControlClient() {
    mutex_ = xSemaphoreCreateMutex();
    queue_ = xQueueCreate(16, sizeof(Command));
}

IpControlClient::~IpControlClient() {
    if (task_ != nullptr) {
        vTaskDelete(task_);
    }
    vQueueDelete(queue_);
    vSemaphoreDelete(mutex_);
}

void IpControlClient::begin() {
    xTaskCreatePinnedToCore(taskEntry, "ipctl", kTaskStackBytes, this, 1, &task_, 0);
}

bool IpControlClient::isValidKeycode(const String& keycode) {
    if (keycode.length() != 8) {
        return false;
    }
    for (size_t i = 0; i < keycode.length(); ++i) {
        const char c = keycode[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))) {
            return false;
        }
    }
    return true;
}

void IpControlClient::configure(const String& host, const String& keycode) {
    MutexLock lock(mutex_);
    host_ = host;
    keycode_ = keycode;
    configChanged_ = true;
    configured_ = !host.isEmpty() && isValidKeycode(keycode);
}

LinkState IpControlClient::state() const {
    if (!configured_) {
        return LinkState::Off;
    }
    return connected_ ? LinkState::Ready : LinkState::Connecting;
}

bool IpControlClient::supports(RemoteKey key) const {
    return findCommand(key) != nullptr;
}

bool IpControlClient::sendKey(RemoteKey key) {
    const char* command = findCommand(key);
    if (command == nullptr || !isReady()) {
        return false;
    }
    enqueue(command);
    return true;
}

void IpControlClient::launchApp(const char* appId) {
    enqueue(String("APP_LAUNCH ") + appId);
}

void IpControlClient::enqueue(const String& command) {
    Command item{};
    strlcpy(item.text, command.c_str(), sizeof(item.text));
    if (xQueueSend(queue_, &item, 0) != pdTRUE) {
        log_w("IP control queue full");
    }
}

void IpControlClient::taskEntry(void* arg) {
    static_cast<IpControlClient*>(arg)->taskLoop();
}

void IpControlClient::taskLoop() {
    for (;;) {
        bool reconfigure = false;
        String keycode;
        {
            MutexLock lock(mutex_);
            if (configChanged_) {
                configChanged_ = false;
                reconfigure = true;
                activeHost_ = host_;
                keycode = keycode_;
            }
        }
        if (reconfigure) {
            client_.stop();
            connected_ = false;
            // PBKDF2 with 16384 iterations takes a moment; it runs once per configuration.
            keyReady_ = configured_ && deriveKey(keycode);
            nextConnectAt_ = 0;
        }

        if (!keyReady_ || !networkAvailable_ || suspended_) {
            if (connected_) {
                client_.stop();
                connected_ = false;
            }
            xQueueReset(queue_);
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        ensureConnected();

        Command command;
        if (xQueueReceive(queue_, &command, pdMS_TO_TICKS(50)) == pdTRUE) {
            if (!sendEncrypted(command.text)) {
                // One retry on a fresh connection: the TV closes idle sockets.
                client_.stop();
                connected_ = false;
                nextConnectAt_ = 0;
                if (ensureConnected()) {
                    sendEncrypted(command.text);
                }
            }
        }
    }
}

bool IpControlClient::ensureConnected() {
    if (client_.connected()) {
        connected_ = true;
        return true;
    }
    connected_ = false;
    if (millis() < nextConnectAt_) {
        return false;
    }
    nextConnectAt_ = millis() + kReconnectIntervalMs;
    if (!client_.connect(activeHost_.c_str(), kPort, kConnectTimeoutMs)) {
        return false;
    }
    client_.setNoDelay(true);
    connected_ = true;
    return true;
}

bool IpControlClient::deriveKey(const String& keycode) {
    mbedtls_md_context_t context;
    mbedtls_md_init(&context);
    bool ok = mbedtls_md_setup(&context, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1) == 0 &&
              mbedtls_pkcs5_pbkdf2_hmac(&context, reinterpret_cast<const unsigned char*>(keycode.c_str()),
                                        keycode.length(), kSalt, sizeof(kSalt), kPbkdf2Iterations, sizeof(key_),
                                        key_) == 0;
    mbedtls_md_free(&context);
    if (!ok) {
        log_e("IP control key derivation failed");
    }
    return ok;
}

bool IpControlClient::sendEncrypted(const char* command) {
    if (!client_.connected()) {
        return false;
    }

    // Message = command + '\r'; a message that is already block-aligned gets an extra
    // space, then PKCS#7-style padding up to the 16-byte block size.
    uint8_t message[64];
    size_t length = strlcpy(reinterpret_cast<char*>(message), command, sizeof(message) - 18);
    message[length++] = '\r';
    if (length % kBlockSize == 0) {
        message[length++] = ' ';
    }
    const uint8_t padding = kBlockSize - (length % kBlockSize);
    if (padding != kBlockSize) {
        memset(message + length, padding, padding);
        length += padding;
    }

    uint8_t iv[kBlockSize];
    esp_fill_random(iv, sizeof(iv));

    // Packet = AES-ECB(iv) || AES-CBC(iv, message).
    uint8_t packet[kBlockSize + sizeof(message)];
    uint8_t chainingIv[kBlockSize];
    memcpy(chainingIv, iv, sizeof(iv));

    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, key_, 128);
    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, iv, packet);
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, length, chainingIv, message, packet + kBlockSize);
    mbedtls_aes_free(&aes);

    const size_t packetLength = kBlockSize + length;
    if (client_.write(packet, packetLength) != packetLength) {
        return false;
    }
    drainResponse();
    return true;
}

void IpControlClient::drainResponse() {
    // The TV answers "OK" (or an error text) encrypted the same way; it is decoded for logging only.
    uint8_t buffer[64];
    size_t received = 0;
    const uint32_t deadline = millis() + kResponseTimeoutMs;
    while (millis() < deadline && received < sizeof(buffer)) {
        const int available = client_.available();
        if (available > 0) {
            received += client_.read(buffer + received, min<size_t>(available, sizeof(buffer) - received));
            if (received >= 2 * kBlockSize && received % kBlockSize == 0) {
                break;
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
    if (received < 2 * kBlockSize || received % kBlockSize != 0) {
        return;
    }

    uint8_t iv[kBlockSize];
    uint8_t plain[sizeof(buffer)];
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_dec(&aes, key_, 128);
    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, buffer, iv);
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, received - kBlockSize, iv, buffer + kBlockSize, plain);
    mbedtls_aes_free(&aes);

    const size_t plainLength = received - kBlockSize;
    for (size_t i = 0; i < plainLength; ++i) {
        if (plain[i] == '\n') {
            plain[i] = '\0';
            log_d("IP control response: %s", reinterpret_cast<char*>(plain));
            return;
        }
    }
}

}  // namespace lgremote
