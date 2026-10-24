// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#include "BleHidTransport.h"

#include <M5Cardputer.h>  // _kb_asciimap: ASCII -> HID usage table

#include "HidReportMap.h"

namespace lgremote {

namespace {

constexpr uint16_t kAppearanceKeyboard = 0x03C1;
constexpr uint8_t kAsciiShiftFlag = 0x80;
constexpr uint32_t kKeyTapMs = 12;

struct BleKey {
    RemoteKey key;
    uint8_t keyboardUsage;   // 0 when the key is sent as consumer control
    uint16_t consumerUsage;  // 0 when the key is sent as keyboard
};

// webOS maps standard HID keys to remote actions: arrows/Enter navigate, Escape is Back.
constexpr BleKey kBleKeys[] = {
    {RemoteKey::Up, hid_key::kUp, 0},
    {RemoteKey::Down, hid_key::kDown, 0},
    {RemoteKey::Left, hid_key::kLeft, 0},
    {RemoteKey::Right, hid_key::kRight, 0},
    {RemoteKey::Ok, hid_key::kEnter, 0},
    {RemoteKey::Back, hid_key::kEscape, 0},
    {RemoteKey::Exit, hid_key::kEscape, 0},
    {RemoteKey::Home, 0, hid_consumer::kAcHome},
    {RemoteKey::Settings, 0, hid_consumer::kMenu},
    {RemoteKey::VolumeUp, 0, hid_consumer::kVolumeUp},
    {RemoteKey::VolumeDown, 0, hid_consumer::kVolumeDown},
    {RemoteKey::Mute, 0, hid_consumer::kMute},
    {RemoteKey::ChannelUp, 0, hid_consumer::kChannelUp},
    {RemoteKey::ChannelDown, 0, hid_consumer::kChannelDown},
    {RemoteKey::PlayPause, 0, hid_consumer::kPlayPause},
    {RemoteKey::Play, 0, hid_consumer::kPlay},
    {RemoteKey::Pause, 0, hid_consumer::kPause},
    {RemoteKey::Stop, 0, hid_consumer::kStop},
    {RemoteKey::Rewind, 0, hid_consumer::kRewind},
    {RemoteKey::FastForward, 0, hid_consumer::kFastForward},
    {RemoteKey::Digit1, hid_key::k1, 0},
    {RemoteKey::Digit2, hid_key::k1 + 1, 0},
    {RemoteKey::Digit3, hid_key::k1 + 2, 0},
    {RemoteKey::Digit4, hid_key::k1 + 3, 0},
    {RemoteKey::Digit5, hid_key::k1 + 4, 0},
    {RemoteKey::Digit6, hid_key::k1 + 5, 0},
    {RemoteKey::Digit7, hid_key::k1 + 6, 0},
    {RemoteKey::Digit8, hid_key::k1 + 7, 0},
    {RemoteKey::Digit9, hid_key::k1 + 8, 0},
    {RemoteKey::Digit0, hid_key::k0, 0},
};

const BleKey* findBleKey(RemoteKey key) {
    for (const BleKey& entry : kBleKeys) {
        if (entry.key == key) {
            return &entry;
        }
    }
    return nullptr;
}

int8_t clampToInt8(int value) {
    return static_cast<int8_t>(constrain(value, -127, 127));
}

}  // namespace

BleHidTransport::BleHidTransport(const char* deviceName) : deviceName_(deviceName) {}

void BleHidTransport::begin() {
    if (started_) {
        return;
    }
    NimBLEDevice::init(deviceName_);
    NimBLEDevice::setSecurityAuth(true, false, true);  // bonding, no MITM (no display on TV side), secure connections
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

    server_ = NimBLEDevice::createServer();
    server_->setCallbacks(this, false);

    hid_ = new NimBLEHIDDevice(server_);
    keyboardInput_ = hid_->getInputReport(kKeyboardReportId);
    mouseInput_ = hid_->getInputReport(kMouseReportId);
    consumerInput_ = hid_->getInputReport(kConsumerReportId);
    hid_->setManufacturer("M5Stack");
    hid_->setPnp(0x02, 0x303A, 0x4C47, 0x0100);  // USB-IF vendor source, Espressif VID, "LG" PID
    hid_->setHidInfo(0x00, 0x01);              // not localized, remote wake
    hid_->setReportMap(const_cast<uint8_t*>(kHidReportMap), sizeof(kHidReportMap));
    hid_->setBatteryLevel(100);

    server_->start();

    NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
    advertising->setAppearance(kAppearanceKeyboard);
    advertising->addServiceUUID(hid_->getHidService()->getUUID());
    advertising->setName(deviceName_);
    advertising->enableScanResponse(true);
    started_ = true;
    startPairing();
}

void BleHidTransport::end() {
    if (!started_) {
        return;
    }
    NimBLEDevice::deinit(true);
    // deinit(true) deletes the server and its services; the HID helper only holds pointers to them.
    delete hid_;
    server_ = nullptr;
    hid_ = nullptr;
    keyboardInput_ = nullptr;
    mouseInput_ = nullptr;
    consumerInput_ = nullptr;
    connected_ = false;
    restartAdvertisingAt_ = 0;
    started_ = false;
}

void BleHidTransport::startPairing() {
    if (!started_) {
        begin();
        return;
    }
    if (!connected_) {
        NimBLEDevice::getAdvertising()->start();
    }
}

void BleHidTransport::forgetBonds() {
    if (!started_) {
        return;
    }
    if (server_ != nullptr && server_->getConnectedCount() > 0) {
        for (uint16_t handle : server_->getPeerDevices()) {
            server_->disconnect(handle);
        }
    }
    NimBLEDevice::deleteAllBonds();
}

bool BleHidTransport::isAdvertising() const {
    return started_ && NimBLEDevice::getAdvertising()->isAdvertising();
}

LinkState BleHidTransport::state() const {
    if (!started_) {
        return LinkState::Off;
    }
    return connected_ ? LinkState::Ready : LinkState::Connecting;
}

bool BleHidTransport::supports(RemoteKey key) const {
    return findBleKey(key) != nullptr;
}

bool BleHidTransport::sendKey(RemoteKey key) {
    const BleKey* entry = findBleKey(key);
    if (entry == nullptr || !connected_) {
        return false;
    }
    if (entry->keyboardUsage != 0) {
        tapKey(entry->keyboardUsage);
    } else {
        tapConsumer(entry->consumerUsage);
    }
    return true;
}

void BleHidTransport::movePointer(int dx, int dy) {
    while (dx != 0 || dy != 0) {
        const int8_t stepX = clampToInt8(dx);
        const int8_t stepY = clampToInt8(dy);
        sendMouse(0, stepX, stepY, 0);
        dx -= stepX;
        dy -= stepY;
    }
}

void BleHidTransport::click() {
    sendMouse(0x01, 0, 0, 0);
    delay(kKeyTapMs);
    sendMouse(0x00, 0, 0, 0);
}

void BleHidTransport::scroll(int dy) {
    // Positive dy scrolls down on the TV; HID wheel positive means "away from the user" (up).
    sendMouse(0, 0, 0, clampToInt8(-dy));
}

void BleHidTransport::typeText(const char* text) {
    for (const char* c = text; *c != '\0'; ++c) {
        const uint8_t ascii = static_cast<uint8_t>(*c);
        if (ascii >= 128) {
            continue;  // non-ASCII cannot be typed on a US HID layout
        }
        const uint8_t mapped = _kb_asciimap[ascii];
        if (mapped == 0) {
            continue;
        }
        const bool shift = (mapped & kAsciiShiftFlag) != 0;
        tapKey(mapped & ~kAsciiShiftFlag, shift ? hid_key::kModLeftShift : 0);
    }
}

void BleHidTransport::deleteText(int count) {
    for (int i = 0; i < count; ++i) {
        tapKey(hid_key::kBackspace);
    }
}

void BleHidTransport::sendEnter() {
    tapKey(hid_key::kEnter);
}

void BleHidTransport::loop() {
    if (started_ && restartAdvertisingAt_ != 0 && millis() >= restartAdvertisingAt_) {
        restartAdvertisingAt_ = 0;
        startPairing();
    }
}

void BleHidTransport::onConnect(NimBLEServer* /*server*/, NimBLEConnInfo& connInfo) {
    connected_ = true;
    log_i("BLE connected: %s", connInfo.getAddress().toString().c_str());
}

void BleHidTransport::onDisconnect(NimBLEServer* /*server*/, NimBLEConnInfo& /*connInfo*/, int reason) {
    connected_ = false;
    log_i("BLE disconnected, reason %d", reason);
    // Advertise again so the TV can reconnect automatically (deferred out of the BLE host callback).
    restartAdvertisingAt_ = millis() + 500;
}

void BleHidTransport::tapKey(uint8_t usage, uint8_t modifiers) {
    if (!connected_) {
        return;
    }
    uint8_t report[8] = {modifiers, 0, usage, 0, 0, 0, 0, 0};
    keyboardInput_->notify(report, sizeof(report));
    delay(kKeyTapMs);
    memset(report, 0, sizeof(report));
    keyboardInput_->notify(report, sizeof(report));
    delay(kKeyTapMs);
}

void BleHidTransport::tapConsumer(uint16_t usage) {
    if (!connected_) {
        return;
    }
    uint8_t report[2] = {static_cast<uint8_t>(usage & 0xFF), static_cast<uint8_t>(usage >> 8)};
    consumerInput_->notify(report, sizeof(report));
    delay(kKeyTapMs);
    report[0] = 0;
    report[1] = 0;
    consumerInput_->notify(report, sizeof(report));
}

void BleHidTransport::sendMouse(uint8_t buttons, int dx, int dy, int wheel) {
    if (!connected_) {
        return;
    }
    uint8_t report[4] = {buttons, static_cast<uint8_t>(clampToInt8(dx)), static_cast<uint8_t>(clampToInt8(dy)),
                         static_cast<uint8_t>(clampToInt8(wheel))};
    mouseInput_->notify(report, sizeof(report));
}

}  // namespace lgremote
