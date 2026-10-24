// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

// All user-visible text in one place (ready for translation).
namespace lgremote::str {

constexpr const char* kAppName = "LG Remote";
constexpr const char* kBleDeviceName = "Cardputer Remote";

// Remote screen
constexpr const char* kIdleTitle = "Ready";
constexpr const char* kIdleHint = "Point at the TV and press a key";
constexpr const char* kFooterRemote = "tab link  opt menu  ? help";
constexpr const char* kFooterPointer = "enter click  fn+;. scroll  a exit";
constexpr const char* kPointerTitle = "POINTER";
constexpr const char* kPointerHintAir = "Move the remote to aim";
constexpr const char* kPointerHintKeys = "Arrow keys move the cursor";
constexpr const char* kVia = "via ";
constexpr const char* kNotSent = "Not available";

// Link feedback
constexpr const char* kNeedsLanOrBle = "Needs LAN or BLE link";
constexpr const char* kBleNotConnected = "Bluetooth not connected";
constexpr const char* kSetUpTvFirst = "Set up TV in menu (opt)";
constexpr const char* kTvNotConnected = "TV not connected";
constexpr const char* kIrOnly = "IR only";
constexpr const char* kBleWaiting = "Waiting for TV";
constexpr const char* kBleConnected = "Connected";
constexpr const char* kBleOff = "Off";

// Menu
constexpr const char* kMenuTitle = "Menu";
constexpr const char* kMenuWifi = "Wi-Fi";
constexpr const char* kMenuTv = "TV (network)";
constexpr const char* kMenuBluetooth = "Bluetooth (beta)";
constexpr const char* kMenuPointer = "Pointer speed";
constexpr const char* kMenuInvertY = "Invert pointer Y";
constexpr const char* kMenuBrightness = "Brightness";
constexpr const char* kMenuKeyClick = "Key click sound";
constexpr const char* kMenuService = "Service codes";
constexpr const char* kMenuAbout = "About";
constexpr const char* kOn = "On";
constexpr const char* kOff = "Off";
constexpr const char* kNotSet = "Not set";
constexpr const char* kFooterList = "enter select  del back";
constexpr const char* kFooterValue = "< > change  del back";

// Wi-Fi
constexpr const char* kWifiTitle = "Wi-Fi network";
constexpr const char* kWifiScanning = "Scanning...";
constexpr const char* kWifiNoNetworks = "No networks found";
constexpr const char* kWifiRescan = "Scan again";
constexpr const char* kWifiForget = "Forget network";
constexpr const char* kWifiPassword = "Password";
constexpr const char* kWifiConnecting = "Connecting to Wi-Fi...";
constexpr const char* kWifiConnected = "Wi-Fi connected";

// TV
constexpr const char* kTvTitle = "TV";
constexpr const char* kTvSearch = "Search for TVs";
constexpr const char* kTvSearching = "Searching the network...";
constexpr const char* kTvManual = "Enter IP address";
constexpr const char* kTvForget = "Forget TV";
constexpr const char* kTvIpPrompt = "TV IP address";
constexpr const char* kTvNoneFound = "No TV found. Enter IP?";
constexpr const char* kTvNeedWifi = "Connect Wi-Fi first";
constexpr const char* kTvMacPrompt = "TV MAC (Wake-on-LAN)";
constexpr const char* kTvMac = "Wake-on-LAN MAC";
constexpr const char* kTvMacInvalid = "Invalid MAC address";
constexpr const char* kTvKeycode = "IP Control keycode";
constexpr const char* kTvKeycodePrompt = "Keycode (TV: Support > IP Control)";
constexpr const char* kTvKeycodeInvalid = "Keycode: 8 letters/digits";
constexpr const char* kTvPairAgain = "Pair again";

// Pairing
constexpr const char* kPairTitle = "Pairing";
constexpr const char* kPairAccept = "Accept the prompt on your TV";
constexpr const char* kPairDone = "Paired!";
constexpr const char* kPairDeclined = "Declined on TV. Try again?";
constexpr const char* kPairConnecting = "Connecting to TV...";
constexpr const char* kPairToast = "Cardputer remote connected";

// Bluetooth
constexpr const char* kBleTitle = "Bluetooth";
constexpr const char* kBleStep1 = "On TV open: Settings > General >";
constexpr const char* kBleStep2 = "External Devices > Bluetooth";
constexpr const char* kBleStep3 = "and pick \"Cardputer Remote\"";
constexpr const char* kBlePair = "Pair with TV";
constexpr const char* kBleForget = "Forget paired TVs";
constexpr const char* kBleKeyTest = "Key test";

// Text entry
constexpr const char* kTypeTitle = "Typing on TV";
constexpr const char* kTypeHint = "TV keyboard open: press t to type";
constexpr const char* kTypeFooter = "enter send  del erase  esc close";
constexpr const char* kFieldFooter = "enter save  esc cancel";
constexpr const char* kRevealHint = "fn+del show / hide";

// Apps
constexpr const char* kAppsTitle = "Apps";
constexpr const char* kLaunching = "Opening ";

// Service
constexpr const char* kServiceTitle = "Service codes (IR)";
constexpr const char* kServiceWarning = "May change TV settings!";
constexpr const char* kConfirmFooter = "enter yes  esc no";

// Help
constexpr const char* kHelpTitle = "Keys";
constexpr const char* kHelpFooter = "< > page  esc close";

// About
constexpr const char* kAboutTitle = "About";
constexpr const char* kAboutVersion = "Version";
constexpr const char* kAboutDevice = "Device";
constexpr const char* kAboutImu = "Motion sensor";
constexpr const char* kAboutIp = "IP address";
constexpr const char* kAboutHeap = "Free memory";
constexpr const char* kAboutAuthor = "Author";
constexpr const char* kAuthor = "Patryk Strzelecki";

// General
constexpr const char* kSaved = "Saved";
constexpr const char* kSent = "Sent: ";

}  // namespace lgremote::str
