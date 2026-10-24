// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

#include <Arduino.h>

#include <vector>

namespace lgremote {

struct WifiNetwork {
    String ssid;
    int32_t rssi;
    bool secured;
};

// Keeps the station connected to the configured network and offers async scanning.
class WifiService {
public:
    void begin();
    void loop();

    void connect(const String& ssid, const String& password);
    void disconnect();

    bool isConnected() const;
    bool isConfigured() const { return ssid_.length() > 0; }
    const String& ssid() const { return ssid_; }
    String localIp() const;

    void startScan();
    bool scanInProgress() const;
    // Returns true once when a scan has finished; results are sorted by signal strength.
    bool takeScanResults(std::vector<WifiNetwork>& out);

private:
    static constexpr uint32_t kRetryIntervalMs = 15000;

    String ssid_;
    String password_;
    uint32_t lastAttemptAt_ = 0;
    bool scanning_ = false;
};

}  // namespace lgremote
