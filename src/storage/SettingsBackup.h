// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

#include <Arduino.h>

namespace lgremote {

struct Settings;

// JSON copy of the settings on the SD card, so a re-install (which may wipe NVS) does not
// lose the Wi-Fi and TV pairing. One file in one folder under the shared /config directory:
//   /config/lgtv-remote/settings.json
// Writes are atomic (temporary file, then rename) so a pulled card never leaves half a file.
class SettingsBackup {
public:
    static constexpr const char* kDirectory = "/config/lgtv-remote";
    static constexpr const char* kPath = "/config/lgtv-remote/settings.json";

    // Mounts the card if present (Cardputer / ADV: SPI on GPIO 40/39/14, CS 12).
    bool mount();
    bool cardPresent() const { return mounted_; }
    bool exists();

    bool save(const Settings& settings);
    bool load(Settings& settings);
    // Deletes the file and the app folder (if empty), leaving the rest of the card untouched.
    bool remove();

private:
    bool ensureMounted();

    bool mounted_ = false;
    uint32_t lastMountAttemptAt_ = 0;
};

}  // namespace lgremote
