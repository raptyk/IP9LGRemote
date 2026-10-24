// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#include "WifiService.h"

#include <WiFi.h>

#include <algorithm>

namespace lgremote {

void WifiService::begin() {
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(true);  // modem sleep keeps BLE coexistence and battery life reasonable
    WiFi.setAutoReconnect(true);
}

void WifiService::connect(const String& ssid, const String& password) {
    ssid_ = ssid;
    password_ = password;
    if (ssid_.isEmpty()) {
        return;
    }
    WiFi.disconnect(false);
    WiFi.begin(ssid_.c_str(), password_.c_str());
    lastAttemptAt_ = millis();
}

void WifiService::disconnect() {
    ssid_ = "";
    password_ = "";
    WiFi.disconnect(false);
}

void WifiService::loop() {
    if (!isConfigured() || scanning_ || isConnected()) {
        return;
    }
    if (millis() - lastAttemptAt_ >= kRetryIntervalMs) {
        WiFi.begin(ssid_.c_str(), password_.c_str());
        lastAttemptAt_ = millis();
    }
}

bool WifiService::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

String WifiService::localIp() const {
    return isConnected() ? WiFi.localIP().toString() : String();
}

void WifiService::startScan() {
    if (scanning_) {
        return;
    }
    WiFi.scanDelete();
    scanning_ = WiFi.scanNetworks(true, false) == WIFI_SCAN_RUNNING;
}

bool WifiService::scanInProgress() const {
    return scanning_;
}

bool WifiService::takeScanResults(std::vector<WifiNetwork>& out) {
    if (!scanning_) {
        return false;
    }
    int16_t count = WiFi.scanComplete();
    if (count == WIFI_SCAN_RUNNING) {
        return false;
    }
    scanning_ = false;
    out.clear();
    for (int16_t i = 0; i < count; ++i) {
        String name = WiFi.SSID(i);
        if (name.isEmpty()) {
            continue;  // hidden network
        }
        auto existing = std::find_if(out.begin(), out.end(), [&](const WifiNetwork& n) { return n.ssid == name; });
        if (existing != out.end()) {
            existing->rssi = std::max(existing->rssi, WiFi.RSSI(i));
            continue;
        }
        out.push_back({name, WiFi.RSSI(i), WiFi.encryptionType(i) != WIFI_AUTH_OPEN});
    }
    std::sort(out.begin(), out.end(), [](const WifiNetwork& a, const WifiNetwork& b) { return a.rssi > b.rssi; });
    WiFi.scanDelete();
    return true;
}

}  // namespace lgremote
