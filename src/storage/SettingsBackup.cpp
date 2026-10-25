// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#include "SettingsBackup.h"

#include <ArduinoJson.h>
#include <SD.h>
#include <SPI.h>

#include "ip9/hw/Pins.h"

#include "app/Settings.h"

namespace lgremote {

namespace {

constexpr uint32_t kSdFrequency = 25000000;
constexpr const char* kTempPath = "/config/lgtv-remote/settings.json.tmp";
constexpr const char* kSharedConfigDirectory = "/config";
// Without a card SD.begin() takes a moment; do not retry on every settings change.
constexpr uint32_t kMountRetryMs = 10000;

}  // namespace

bool SettingsBackup::mount() {
    if (mounted_) {
        return true;
    }
    lastMountAttemptAt_ = millis();
    SPI.begin(ip9::pins::kSdSck, ip9::pins::kSdMiso, ip9::pins::kSdMosi, ip9::pins::kSdCs);
    mounted_ = SD.begin(ip9::pins::kSdCs, SPI, kSdFrequency);
    if (!mounted_) {
        SD.end();
    }
    return mounted_;
}

bool SettingsBackup::ensureMounted() {
    // The card can be inserted after start-up; also detect a card that was pulled out.
    if (mounted_ && SD.cardType() == CARD_NONE) {
        SD.end();
        mounted_ = false;
    }
    if (!mounted_ && lastMountAttemptAt_ != 0 && millis() - lastMountAttemptAt_ < kMountRetryMs) {
        return false;
    }
    return mount();
}

bool SettingsBackup::exists() {
    return ensureMounted() && SD.exists(kPath);
}

bool SettingsBackup::save(const Settings& settings) {
    if (!ensureMounted()) {
        return false;
    }
    if (!SD.exists(kSharedConfigDirectory)) {
        SD.mkdir(kSharedConfigDirectory);
    }
    if (!SD.exists(kDirectory)) {
        SD.mkdir(kDirectory);
    }

    JsonDocument doc;
    settings.toJson(doc.to<JsonObject>());

    File file = SD.open(kTempPath, FILE_WRITE);
    if (!file) {
        log_w("Settings backup: cannot write %s", kTempPath);
        return false;
    }
    const size_t written = serializeJsonPretty(doc, file);
    file.close();
    if (written == 0) {
        SD.remove(kTempPath);
        return false;
    }
    // FAT cannot rename over an existing file: replace it only once the new copy is complete.
    if (SD.exists(kPath)) {
        SD.remove(kPath);
    }
    return SD.rename(kTempPath, kPath);
}

bool SettingsBackup::load(Settings& settings) {
    if (!ensureMounted()) {
        return false;
    }
    // A leftover temporary file means a save was interrupted: the old file is still valid.
    const char* path = SD.exists(kPath) ? kPath : (SD.exists(kTempPath) ? kTempPath : nullptr);
    if (path == nullptr) {
        return false;
    }
    File file = SD.open(path, FILE_READ);
    if (!file) {
        return false;
    }
    JsonDocument doc;
    const DeserializationError error = deserializeJson(doc, file);
    file.close();
    if (error) {
        log_w("Settings backup: invalid JSON (%s)", error.c_str());
        return false;
    }
    return settings.fromJson(doc.as<JsonObjectConst>());
}

bool SettingsBackup::remove() {
    if (!ensureMounted()) {
        return false;
    }
    bool ok = true;
    if (SD.exists(kPath)) {
        ok = SD.remove(kPath);
    }
    if (SD.exists(kTempPath)) {
        SD.remove(kTempPath);
    }
    SD.rmdir(kDirectory);              // only succeeds when empty
    SD.rmdir(kSharedConfigDirectory);  // likewise; other apps' folders stay
    return ok;
}

}  // namespace lgremote
