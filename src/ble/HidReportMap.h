// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

#include <stdint.h>

namespace lgremote {

constexpr uint8_t kKeyboardReportId = 1;
constexpr uint8_t kMouseReportId = 2;
constexpr uint8_t kConsumerReportId = 3;

// Composite HID: boot-like keyboard, 3-button mouse with wheel, 16-bit consumer control.
static const uint8_t kHidReportMap[] = {
    // Keyboard
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
    0x85, kKeyboardReportId,
    0x05, 0x07,        //   Usage Page (Keyboard/Keypad)
    0x19, 0xE0,        //   Usage Minimum (Left Control)
    0x29, 0xE7,        //   Usage Maximum (Right GUI)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x02,        //   Input (Data, Variable, Absolute) - modifiers
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x08,        //   Report Size (8)
    0x81, 0x01,        //   Input (Constant) - reserved
    0x95, 0x06,        //   Report Count (6)
    0x75, 0x08,        //   Report Size (8)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x73,        //   Logical Maximum (115)
    0x05, 0x07,        //   Usage Page (Keyboard/Keypad)
    0x19, 0x00,        //   Usage Minimum (0)
    0x29, 0x73,        //   Usage Maximum (115)
    0x81, 0x00,        //   Input (Data, Array, Absolute) - key codes
    0xC0,              // End Collection

    // Mouse
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x02,        // Usage (Mouse)
    0xA1, 0x01,        // Collection (Application)
    0x85, kMouseReportId,
    0x09, 0x01,        //   Usage (Pointer)
    0xA1, 0x00,        //   Collection (Physical)
    0x05, 0x09,        //     Usage Page (Buttons)
    0x19, 0x01,        //     Usage Minimum (1)
    0x29, 0x03,        //     Usage Maximum (3)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x01,        //     Logical Maximum (1)
    0x95, 0x03,        //     Report Count (3)
    0x75, 0x01,        //     Report Size (1)
    0x81, 0x02,        //     Input (Data, Variable, Absolute)
    0x95, 0x01,        //     Report Count (1)
    0x75, 0x05,        //     Report Size (5)
    0x81, 0x03,        //     Input (Constant) - padding
    0x05, 0x01,        //     Usage Page (Generic Desktop)
    0x09, 0x30,        //     Usage (X)
    0x09, 0x31,        //     Usage (Y)
    0x09, 0x38,        //     Usage (Wheel)
    0x15, 0x81,        //     Logical Minimum (-127)
    0x25, 0x7F,        //     Logical Maximum (127)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x03,        //     Report Count (3)
    0x81, 0x06,        //     Input (Data, Variable, Relative)
    0xC0,              //   End Collection
    0xC0,              // End Collection

    // Consumer control
    0x05, 0x0C,        // Usage Page (Consumer)
    0x09, 0x01,        // Usage (Consumer Control)
    0xA1, 0x01,        // Collection (Application)
    0x85, kConsumerReportId,
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x03,  //   Logical Maximum (1023)
    0x19, 0x00,        //   Usage Minimum (0)
    0x2A, 0xFF, 0x03,  //   Usage Maximum (1023)
    0x75, 0x10,        //   Report Size (16)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x00,        //   Input (Data, Array, Absolute)
    0xC0,              // End Collection
};

// HID keyboard usage IDs (Usage Page 0x07).
namespace hid_key {
constexpr uint8_t kA = 0x04;
constexpr uint8_t k1 = 0x1E;
constexpr uint8_t k0 = 0x27;
constexpr uint8_t kEnter = 0x28;
constexpr uint8_t kEscape = 0x29;
constexpr uint8_t kBackspace = 0x2A;
constexpr uint8_t kTab = 0x2B;
constexpr uint8_t kSpace = 0x2C;
constexpr uint8_t kF1 = 0x3A;
constexpr uint8_t kHome = 0x4A;
constexpr uint8_t kPageUp = 0x4B;
constexpr uint8_t kPageDown = 0x4E;
constexpr uint8_t kRight = 0x4F;
constexpr uint8_t kLeft = 0x50;
constexpr uint8_t kDown = 0x51;
constexpr uint8_t kUp = 0x52;
constexpr uint8_t kMenu = 0x76;
constexpr uint8_t kModLeftShift = 0x02;
}  // namespace hid_key

// HID consumer usage IDs (Usage Page 0x0C).
namespace hid_consumer {
constexpr uint16_t kPower = 0x0030;
constexpr uint16_t kMenu = 0x0040;
constexpr uint16_t kChannelUp = 0x009C;
constexpr uint16_t kChannelDown = 0x009D;
constexpr uint16_t kPlay = 0x00B0;
constexpr uint16_t kPause = 0x00B1;
constexpr uint16_t kFastForward = 0x00B3;
constexpr uint16_t kRewind = 0x00B4;
constexpr uint16_t kStop = 0x00B7;
constexpr uint16_t kPlayPause = 0x00CD;
constexpr uint16_t kMute = 0x00E2;
constexpr uint16_t kVolumeUp = 0x00E9;
constexpr uint16_t kVolumeDown = 0x00EA;
constexpr uint16_t kAcHome = 0x0223;
constexpr uint16_t kAcBack = 0x0224;
}  // namespace hid_consumer

}  // namespace lgremote
