// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#include "UsbHidTransport.h"

#include <Arduino.h>
#include <USB.h>
#include <USBHIDConsumerControl.h>
#include <USBHIDKeyboard.h>
#include <USBHIDMouse.h>

#include "ble/HidReportMap.h"
#include "ui/Strings.h"

namespace lgremote {

namespace {

// The HID devices register their report descriptors when constructed, which must happen
// before USB.begin(); file-scope objects guarantee that.
USBHIDKeyboard usbKeyboard;
USBHIDMouse usbMouse;
USBHIDConsumerControl usbConsumer;

UsbHidTransport* activeTransport = nullptr;

constexpr uint32_t kKeyTapMs = 12;

struct UsbKey {
    RemoteKey key;
    uint8_t keyboardUsage;   // 0 when sent as consumer control
    uint16_t consumerUsage;  // 0 when sent as keyboard
};

// Same mapping as the Bluetooth HID mode: webOS treats USB and BT keyboards alike.
constexpr UsbKey kUsbKeys[] = {
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

const UsbKey* findUsbKey(RemoteKey key) {
    for (const UsbKey& entry : kUsbKeys) {
        if (entry.key == key) {
            return &entry;
        }
    }
    return nullptr;
}

void onUsbEvent(void* /*arg*/, esp_event_base_t base, int32_t id, void* /*data*/) {
    if (base != ARDUINO_USB_EVENTS || activeTransport == nullptr) {
        return;
    }
    switch (id) {
        case ARDUINO_USB_STARTED_EVENT:
        case ARDUINO_USB_RESUME_EVENT:
            activeTransport->setMounted(true);
            break;
        case ARDUINO_USB_STOPPED_EVENT:
        case ARDUINO_USB_SUSPEND_EVENT:
            activeTransport->setMounted(false);
            break;
        default:
            break;
    }
}

int8_t clampToInt8(int value) {
    return static_cast<int8_t>(constrain(value, -127, 127));
}

}  // namespace

void UsbHidTransport::begin() {
    if (started_) {
        return;
    }
    activeTransport = this;
    usbKeyboard.begin();
    usbMouse.begin();
    usbConsumer.begin();
    USB.onEvent(onUsbEvent);
    USB.productName(str::kBleDeviceName);
    USB.manufacturerName("M5Stack");
    USB.begin();
    started_ = true;
}

LinkState UsbHidTransport::state() const {
    if (!started_) {
        return LinkState::Off;
    }
    return mounted_ ? LinkState::Ready : LinkState::Connecting;
}

bool UsbHidTransport::supports(RemoteKey key) const {
    return findUsbKey(key) != nullptr;
}

bool UsbHidTransport::sendKey(RemoteKey key) {
    const UsbKey* entry = findUsbKey(key);
    if (entry == nullptr || !isReady()) {
        return false;
    }
    if (entry->keyboardUsage != 0) {
        tapKey(entry->keyboardUsage);
    } else {
        tapConsumer(entry->consumerUsage);
    }
    return true;
}

void UsbHidTransport::movePointer(int dx, int dy) {
    if (!isReady()) {
        return;
    }
    while (dx != 0 || dy != 0) {
        const int8_t stepX = clampToInt8(dx);
        const int8_t stepY = clampToInt8(dy);
        usbMouse.move(stepX, stepY);
        dx -= stepX;
        dy -= stepY;
    }
}

void UsbHidTransport::click() {
    if (isReady()) {
        usbMouse.click(MOUSE_LEFT);
    }
}

void UsbHidTransport::scroll(int dy) {
    if (isReady()) {
        // Positive dy scrolls down; the HID wheel counts "up" as positive.
        usbMouse.move(0, 0, clampToInt8(-dy));
    }
}

void UsbHidTransport::typeText(const char* text) {
    if (isReady()) {
        usbKeyboard.print(text);  // US layout ASCII -> HID handled by the core
    }
}

void UsbHidTransport::deleteText(int count) {
    for (int i = 0; i < count; ++i) {
        tapKey(hid_key::kBackspace);
    }
}

void UsbHidTransport::sendEnter() {
    tapKey(hid_key::kEnter);
}

void UsbHidTransport::tapKey(uint8_t usage) {
    if (!isReady()) {
        return;
    }
    usbKeyboard.pressRaw(usage);
    delay(kKeyTapMs);
    usbKeyboard.releaseRaw(usage);
}

void UsbHidTransport::tapConsumer(uint16_t usage) {
    if (!isReady()) {
        return;
    }
    usbConsumer.press(usage);
    delay(kKeyTapMs);
    usbConsumer.release();
}

}  // namespace lgremote
