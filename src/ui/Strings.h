// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

// All user-visible text in one place (ready for translation).
// Screen budget: 240 px. Footers use the 6 px font (max ~38 characters);
// long texts are word-wrapped by the painter.
namespace lgremote::str {

constexpr const char* kAppName = "IP9 LG Remote";
constexpr const char* kBleDeviceName = "Cardputer Remote";

// Home launcher
constexpr const char* kHomeStatus = "Choose a mode";
constexpr const char* kChipHome = "LG";
constexpr const char* kFooterHome = "<> pick  enter open  s setup  tab help";
constexpr const char* kCardWifiTitle = "Wi-Fi remote";
constexpr const char* kCardWifiText = "Pointer, typing and apps over your home network. Falls back to IR.";
constexpr const char* kCardIrTitle = "IR remote";
constexpr const char* kCardIrText = "Classic infrared remote. Works with every LG TV, no Wi-Fi needed.";
constexpr const char* kCardBleTitle = "Bluetooth";
constexpr const char* kCardBleText = "Keyboard and mouse for TVs with webOS 5 (2020) or newer. Beta.";
constexpr const char* kCardUsbTitle = "USB cable";
constexpr const char* kCardUsbText = "Keyboard and mouse over a USB cable. Also for older TVs.";
constexpr const char* kCardSettingsTitle = "Settings";
constexpr const char* kCardSettingsText = "Wi-Fi, TV, pointer speed, screen and service codes.";
constexpr const char* kCardHelpTitle = "Help";
constexpr const char* kCardHelpText = "All remote keys at a glance.";
constexpr const char* kReady = "Ready";
constexpr const char* kSetupNeeded = "Setup needed - press enter";

// Setup checklists
constexpr const char* kSetupTitlePrefix = "Setup: ";
constexpr const char* kStepDone = "Done";
constexpr const char* kStepOk = "OK";
constexpr const char* kStepStart = "Start remote";
constexpr const char* kStepStartText = "Opens the remote. Setup can be repeated: home screen, key s.";
constexpr const char* kStepFirstPrefix = "First: ";

constexpr const char* kStepWifi = "1. Connect to Wi-Fi";
constexpr const char* kStepWifiText = "The Cardputer must join the same network as the TV.";
constexpr const char* kStepTvPrep = "2. Prepare the TV";
constexpr const char* kStepTvPrepText = "TV on. Turn on LG Connect Apps (Network) or Mobile TV On (General).";
constexpr const char* kStepFindTv = "3. Find the TV";
constexpr const char* kStepFindTvText = "Search the network, or type the IP (TV: Settings > Network > Advanced).";
constexpr const char* kStepPair = "4. Pair";
constexpr const char* kStepPairText = "TV asks to allow the remote (or shows a PIN). Nothing else to type.";

constexpr const char* kStepIrTest = "Test infrared (optional)";
constexpr const char* kStepIrTestText = "Aim the top edge (IR LED) at the TV and press enter to send VOL+.";
constexpr const char* kIrTestQuestion = "Did the TV volume go up?";
constexpr const char* kIrTestDetail = "enter = yes, esc = no";
constexpr const char* kIrTestTips = "Get closer and aim at the TV's lower edge";
constexpr const char* kIrTestOk = "Infrared works";
constexpr const char* kSetupDoneTitle = "Connected!";
constexpr const char* kSetupDoneBle = "Bluetooth paired with the TV";
constexpr const char* kSetupDoneUsb = "TV detected the USB remote";
constexpr const char* kSetupDoneHint = "TAB shows all keys";

constexpr const char* kStepBleCheck = "1. Check the TV";
constexpr const char* kStepBleCheckText = "Needs webOS 5 (2020+) with Bluetooth. Older TV: use the USB cable mode.";
constexpr const char* kStepBlePair = "2. Pair with the TV";
constexpr const char* kStepBlePairText = "Shows where to find the Bluetooth menu on the TV and waits for it.";

constexpr const char* kStepUsbCable = "1. Connect the cable";
constexpr const char* kStepUsbCableText = "USB-C cable from the Cardputer to the TV's USB port. Enter starts USB.";
constexpr const char* kUsbWaiting = "Waiting";
constexpr const char* kUsbNotConnected = "Plug into TV USB";

// Remote screen
constexpr const char* kBandWifi = "WI-FI REMOTE";
constexpr const char* kBandIr = "IR REMOTE";
constexpr const char* kBandBle = "BLUETOOTH REMOTE";
constexpr const char* kBandUsb = "USB REMOTE";
constexpr const char* kBandService = "SERVICE MENU (IR)";
constexpr const char* kStatusConnected = "Connected";
constexpr const char* kStatusConnecting = "Connecting...";
constexpr const char* kStatusWaitingTv = "Waiting for the TV";
constexpr const char* kStatusIrFallbackDetail = "Keys go over IR meanwhile: aim at the TV.";
constexpr const char* kStatusInfrared = "Infrared";
constexpr const char* kStatusInfraredDetail = "Aim the top edge (IR LED) at the TV.";
constexpr const char* kStatusBleDetail = "Bluetooth keyboard and mouse";
constexpr const char* kStatusBleWaitingDetail = "Pair it in the TV's Bluetooth menu.";
constexpr const char* kStatusUsbDetail = "USB keyboard and mouse";
constexpr const char* kStatusUsbWaitingDetail = "Plug the cable into a USB port of the TV.";
constexpr const char* kKeyTab = "TAB";
constexpr const char* kKeyOpt = "OPT";
constexpr const char* kKeyA = "A";
constexpr const char* kKeyEnter = "ENTER";
constexpr const char* kKeyBrackets = "[ ]";
constexpr const char* kKeyHelp = "Help";
constexpr const char* kKeyMenu = "Menu";
constexpr const char* kKeyBack = "Back";
constexpr const char* kKeyAirMouse = "Air mouse";
constexpr const char* kKeyPointer = "Pointer";
constexpr const char* kKeyClick = "Click";
constexpr const char* kKeyScroll = "Scroll";
constexpr const char* kKeyExit = "Exit";
constexpr const char* kFooterRemote = "t type  l apps  p power  0-9 digits";
constexpr const char* kFooterRemoteIr = "p power  = - volume  ] [ channel";
constexpr const char* kFooterServiceKeypad = "0-9 digits  enter OK  del back";
constexpr const char* kFooterPointer = "del back  esc exit pointer";
constexpr const char* kPointerTitle = "POINTER";
constexpr const char* kAirMouseTitle = "AIR MOUSE";
constexpr const char* kPointerHintAir = "Move the remote to aim";
constexpr const char* kPointerHintKeys = "Arrow keys move the cursor";
constexpr const char* kVia = "via ";
constexpr const char* kNotSent = "Not available";

// Status bar
constexpr const char* kInfrared = "Infrared";
constexpr const char* kViaIrSuffix = " (IR)";

// Link feedback
constexpr const char* kNeedsLanOrBle = "IR has no pointer or typing";
constexpr const char* kBleNotConnected = "Bluetooth not connected";
constexpr const char* kSetUpTvFirst = "Set up the TV first";
constexpr const char* kTvNotConnected = "TV not connected";
constexpr const char* kBleWaiting = "Waiting for TV";
constexpr const char* kBleConnected = "Connected";
constexpr const char* kBleOff = "Off";

// Settings menu
constexpr const char* kMenuTitle = "Settings";
constexpr const char* kMenuWifi = "Wi-Fi";
constexpr const char* kMenuWifiText = "Network the Cardputer joins. Must be the same as the TV's.";
constexpr const char* kMenuTv = "TV (network)";
constexpr const char* kMenuTvText = "TV address, pairing, IP Control keycode, Wake-on-LAN.";
constexpr const char* kMenuBluetooth = "Bluetooth (beta)";
constexpr const char* kMenuBluetoothText = "Pairing, key test and forgetting paired TVs.";
constexpr const char* kMenuPointer = "Pointer speed";
constexpr const char* kMenuPointerText = "How fast the cursor moves (air mouse and arrows). < > change.";
constexpr const char* kMenuInvertY = "Invert pointer Y";
constexpr const char* kMenuInvertYText = "Flip up/down if the cursor moves the wrong way.";
constexpr const char* kMenuBrightness = "Brightness";
constexpr const char* kMenuBrightnessText = "Screen brightness. Lower saves battery. < > change.";
constexpr const char* kMenuKeyClick = "Key click sound";
constexpr const char* kMenuKeyClickText = "Short beep on every key press.";
constexpr const char* kMenuStatusLed = "Status light";
constexpr const char* kMenuStatusLedText = "LED flash = sent, pulse = connecting. ADV: keeps screen at max.";
constexpr const char* kBrightnessLocked = "Max (LED)";
constexpr const char* kBrightnessLockedToast = "ADV: the LED shares the screen power. Turn Status light off to dim.";
constexpr const char* kMenuTestLed = "Test status light";
constexpr const char* kMenuTestLedText = "Shows red, green, blue and white on the LED.";
constexpr const char* kTestLedOff = "Status light is off";
constexpr const char* kMenuBackup = "SD card backup";
constexpr const char* kMenuBackupText = "Keeps Wi-Fi and TV pairing on the SD card, so a re-install keeps them.";
constexpr const char* kBackupTitle = "SD card backup";
constexpr const char* kBackupAuto = "Automatic copy";
constexpr const char* kBackupAutoText = "Copy in /config/lgtv-remote on the card. Contains the Wi-Fi password!";
constexpr const char* kBackupSaveNow = "Save now";
constexpr const char* kBackupSaveNowText = "Writes the current settings to the card.";
constexpr const char* kBackupRestore = "Restore from card";
constexpr const char* kBackupRestoreText = "Replaces the current settings with the copy on the card.";
constexpr const char* kBackupDelete = "Delete from card";
constexpr const char* kBackupDeleteText = "Removes the file (and its empty folder) and stops the copy.";
constexpr const char* kBackupSaved = "Saved to SD card";
constexpr const char* kBackupRestored = "Settings restored from SD card";
constexpr const char* kBackupDeleted = "Backup deleted from SD card";
constexpr const char* kBackupNoCard = "No SD card inserted";
constexpr const char* kBackupNoCardShort = "No card";
constexpr const char* kBackupMissing = "No backup on the SD card";
constexpr const char* kMenuService = "Service codes";
constexpr const char* kMenuServiceText = "Discrete IR codes: power on/off, HDMI inputs, service menus.";
constexpr const char* kMenuAbout = "About";
constexpr const char* kMenuAboutText = "Version, device, network address, memory.";
constexpr const char* kOn = "On";
constexpr const char* kOff = "Off";
constexpr const char* kNotSet = "Not set";

// Wi-Fi
constexpr const char* kWifiTitle = "Wi-Fi network";
constexpr const char* kWifiScanning = "Searching for Wi-Fi";
constexpr const char* kWifiScanningText = "This takes a few seconds, please wait.";
constexpr const char* kWifiStillScanning = "Still searching (try %d of %d)...";
constexpr const char* kWifiNoNetworks = "No networks found";
constexpr const char* kWifiRescan = "Search again";
constexpr const char* kWifiRescanText = "Nothing found yet. Check the router is on and nearby, then enter.";
constexpr const char* kWifiRescanListText = "Refreshes the list of networks.";
constexpr const char* kWifiForget = "Forget network";
constexpr const char* kWifiPassword = "Password";
constexpr const char* kWifiConnecting = "Connecting to Wi-Fi...";
constexpr const char* kWifiPickText = "Signal strength on the right, * = password protected.";

// TV
constexpr const char* kTvTitle = "TV (network)";
constexpr const char* kTvSearch = "Search for TVs";
constexpr const char* kTvSearchText = "Finds LG webOS TVs on this network. The TV must be on.";
constexpr const char* kTvSearching = "Searching the network...";
constexpr const char* kTvManual = "Enter IP address";
constexpr const char* kTvManualText = "TV: Settings > Network > Wi-Fi / Wired > Advanced shows the IP.";
constexpr const char* kTvPairingMethod = "Pairing method";
constexpr const char* kTvPairingMethodText = "Prompt: press Allow on the TV. PIN: type the code the TV shows.";
constexpr const char* kTvMethodPrompt = "Prompt";
constexpr const char* kTvMethodPin = "PIN";
constexpr const char* kTvForget = "Forget TV";
constexpr const char* kTvForgetText = "Removes address, pairing and keycode from the Cardputer.";
constexpr const char* kTvIpPrompt = "TV IP address";
constexpr const char* kTvIpHint = "Example: 192.168.1.20";
constexpr const char* kTvIpInvalid = "Not a valid IP address";
constexpr const char* kTvNeedWifi = "Connect Wi-Fi first";
constexpr const char* kTvMacPrompt = "TV MAC (Wake-on-LAN)";
constexpr const char* kTvMacHint = "Learned automatically after pairing. Format AA:BB:CC:DD:EE:FF";
constexpr const char* kTvMac = "Wake-on-LAN MAC";
constexpr const char* kTvMacText = "Lets the network turn the TV on (TV: Turn on via Wi-Fi).";
constexpr const char* kTvMacInvalid = "Invalid MAC address";
constexpr const char* kTvKeycode = "IP Control keycode";
constexpr const char* kTvKeycodeText = "Optional, webOS 23+: Settings > Support > IP Control shows 8 chars.";
constexpr const char* kTvKeycodePrompt = "IP Control keycode";
constexpr const char* kTvKeycodeHint = "8 letters/digits from TV: Support > IP Control Settings. Not the dev-mode code.";
constexpr const char* kTvKeycodeInvalid = "Keycode: 8 letters/digits";
constexpr const char* kTvPairAgain = "Pair again";
constexpr const char* kTvPairAgainText = "Asks the TV for a new pairing (after a reset or a declined prompt).";
constexpr const char* kTvSearchResultText = "Enter pairs with this TV.";

// Pairing
constexpr const char* kPairConnectingTitle = "Connecting to the TV";
constexpr const char* kPairConnectingText = "Make sure the TV is on and on the same network.";
constexpr const char* kPairAcceptTitle = "Accept on your TV";
constexpr const char* kPairAcceptText = "Choose Allow / Yes with the TV remote. No prompt? press p for PIN.";
constexpr const char* kPairPinTitle = "Enter the PIN";
constexpr const char* kPairPinText = "Type the code shown on the TV screen. Press enter to type it.";
constexpr const char* kPairPinPrompt = "PIN shown on the TV";
constexpr const char* kPairDone = "Paired!";
constexpr const char* kPairFailedTitle = "Pairing failed";
constexpr const char* kPairUnreachableTitle = "TV not answering";
constexpr const char* kPairChecks = "Check: TV on, same Wi-Fi, LG Connect Apps on.";
constexpr const char* kPairNoWifiTitle = "No Wi-Fi";
constexpr const char* kPairNoWifiText = "Connect the Cardputer to Wi-Fi first.";
constexpr const char* kPairToast = "Cardputer remote connected";
constexpr const char* kFooterPairing = "esc back";
constexpr const char* kFooterPairingAccept = "p use PIN  esc back";
constexpr const char* kFooterPairingPin = "enter type PIN  esc back";
constexpr const char* kFooterPairingFailed = "enter retry  p PIN  esc back";
constexpr const char* kPairMethodPin = "Pairing with PIN";
constexpr const char* kPairMethodPrompt = "Pairing with prompt";

// Bluetooth
constexpr const char* kBleTitle = "Bluetooth pairing";
constexpr const char* kBleHowTo =
    "On the TV open Settings > General > External Devices > Bluetooth Controller "
    "(webOS 5: Connection > Device Connection Settings) and pick \"Cardputer Remote\".";
constexpr const char* kBlePair = "Pair with TV";
constexpr const char* kBlePairText = "Makes the Cardputer visible to the TV and shows the steps.";
constexpr const char* kBleForget = "Forget paired TVs";
constexpr const char* kBleForgetText = "Removes all Bluetooth bonds stored on the Cardputer.";
constexpr const char* kBleKeyTest = "Key test";
constexpr const char* kBleKeyTestText = "Sends single HID keys to see which ones your TV understands.";

// Text entry
constexpr const char* kTypeTitle = "Typing on TV";
constexpr const char* kTypeHint = "TV keyboard open: press t to type";
constexpr const char* kTypeFooter = "enter send  del erase  esc close";
constexpr const char* kFieldFooter = "enter save  esc cancel";
constexpr const char* kRevealHint = "fn+del show / hide";

// Apps
constexpr const char* kAppsTitle = "Apps";
constexpr const char* kAppsIrText = "Works over IR too (dedicated remote key).";
constexpr const char* kAppsLanText = "Needs the Wi-Fi link (or IP Control).";
constexpr const char* kLaunching = "Opening ";

// Service
constexpr const char* kServiceTitle = "Service codes (IR)";
constexpr const char* kServiceWarning = "Password: 0413 (else 0000). Wrong changes can break the TV!";
constexpr const char* kServiceKeypadTitle = "Service menu (IR)";
constexpr const char* kServiceKeypadText = "Password usually 0413 (else 0000). Digits, arrows and enter go over IR.";

// Help
constexpr const char* kHelpTitle = "Keys";

// About
constexpr const char* kAboutTitle = "About";
constexpr const char* kAboutVersion = "Version";
constexpr const char* kAboutDevice = "Device";
constexpr const char* kAboutImu = "Motion sensor";
constexpr const char* kAboutIp = "IP address";
constexpr const char* kAboutHeap = "Free memory";
constexpr const char* kAboutAuthor = "Author";
constexpr const char* kAboutAuthorText = "Who made this remote. Enter shows the details.";
constexpr const char* kAuthor = "Patryk Strzelecki";
constexpr const char* kAuthorEmail = "patryk@strzelecki.info";
constexpr const char* kAuthorDetail = "IP9 LG Remote, MIT license";
constexpr const char* kAboutWebsite = "Website";
constexpr const char* kAboutWebsiteText = "More IP9 apps for the Cardputer and updates.";

// General
constexpr const char* kSaved = "Saved";
constexpr const char* kSent = "Sent: ";

}  // namespace lgremote::str
