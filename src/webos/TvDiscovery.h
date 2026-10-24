// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

#include <Arduino.h>
#include <WiFiUdp.h>

#include <vector>

namespace lgremote {

struct DiscoveredTv {
    String host;
    String name;
};

// Finds LG webOS TVs on the local network with an SSDP M-SEARCH.
// Non-blocking: call start(), then poll() from the main loop until it returns false.
class TvDiscovery {
public:
    void start();
    // Returns true while the search is still running.
    bool poll();
    void cancel();
    bool running() const { return running_; }
    const std::vector<DiscoveredTv>& results() const { return results_; }

private:
    static constexpr uint32_t kSearchDurationMs = 3500;

    void sendSearch();
    void handleResponse(const String& response, const IPAddress& sender);
    static String headerValue(const String& response, const char* name);
    static String fetchFriendlyName(const String& location);

    WiFiUDP udp_;
    bool running_ = false;
    uint32_t startedAt_ = 0;
    uint8_t searchesSent_ = 0;
    std::vector<DiscoveredTv> results_;
    std::vector<String> pendingLocations_;
};

}  // namespace lgremote
