// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#include "TvDiscovery.h"

#include <HTTPClient.h>
#include <WiFi.h>

#include <algorithm>

namespace lgremote {

namespace {

const IPAddress kSsdpGroup(239, 255, 255, 250);
constexpr uint16_t kSsdpPort = 1900;
constexpr const char* kSearchTarget = "urn:lge-com:service:webos-second-screen:1";

}  // namespace

void TvDiscovery::start() {
    cancel();
    results_.clear();
    pendingLocations_.clear();
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }
    udp_.begin(0);
    running_ = true;
    startedAt_ = millis();
    searchesSent_ = 0;
    sendSearch();
}

void TvDiscovery::cancel() {
    if (running_) {
        udp_.stop();
    }
    running_ = false;
}

void TvDiscovery::sendSearch() {
    // UDP is lossy, so the request is repeated a few times during the search window.
    String request = String("M-SEARCH * HTTP/1.1\r\n") + "HOST: 239.255.255.250:1900\r\n" +
                     "MAN: \"ssdp:discover\"\r\n" + "MX: 2\r\n" + "ST: " + kSearchTarget + "\r\n\r\n";
    udp_.beginPacket(kSsdpGroup, kSsdpPort);
    udp_.write(reinterpret_cast<const uint8_t*>(request.c_str()), request.length());
    udp_.endPacket();
    ++searchesSent_;
}

bool TvDiscovery::poll() {
    if (!running_) {
        return false;
    }
    const uint32_t elapsed = millis() - startedAt_;

    int size = udp_.parsePacket();
    while (size > 0) {
        String response;
        response.reserve(size);
        char buffer[256];
        int read;
        while ((read = udp_.read(buffer, sizeof(buffer) - 1)) > 0) {
            buffer[read] = '\0';
            response += buffer;
        }
        handleResponse(response, udp_.remoteIP());
        size = udp_.parsePacket();
    }

    if (searchesSent_ < 3 && elapsed > searchesSent_ * 1000u) {
        sendSearch();
    }

    if (elapsed < kSearchDurationMs) {
        return true;
    }

    udp_.stop();
    // Friendly names come from the UPnP description; fetched at the end so packets are not missed.
    for (size_t i = 0; i < results_.size() && i < pendingLocations_.size(); ++i) {
        String name = fetchFriendlyName(pendingLocations_[i]);
        if (!name.isEmpty()) {
            results_[i].name = name;
        }
    }
    running_ = false;
    return false;
}

void TvDiscovery::handleResponse(const String& response, const IPAddress& sender) {
    if (response.indexOf(kSearchTarget) < 0 && response.indexOf("webOS") < 0 && response.indexOf("WebOS") < 0) {
        return;
    }
    const String host = sender.toString();
    const bool known = std::any_of(results_.begin(), results_.end(), [&](const DiscoveredTv& tv) { return tv.host == host; });
    if (known) {
        return;
    }
    results_.push_back({host, String("LG TV ") + host});
    pendingLocations_.push_back(headerValue(response, "LOCATION"));
}

String TvDiscovery::headerValue(const String& response, const char* name) {
    String lower = response;
    lower.toLowerCase();
    String key = String(name) + ":";
    key.toLowerCase();
    int start = lower.indexOf(key);
    if (start < 0) {
        return String();
    }
    start += key.length();
    int end = response.indexOf("\r\n", start);
    String value = response.substring(start, end < 0 ? response.length() : end);
    value.trim();
    return value;
}

String TvDiscovery::fetchFriendlyName(const String& location) {
    if (!location.startsWith("http://")) {
        return String();
    }
    HTTPClient http;
    http.setConnectTimeout(1500);
    http.setTimeout(1500);
    if (!http.begin(location)) {
        return String();
    }
    String name;
    if (http.GET() == HTTP_CODE_OK) {
        String body = http.getString();
        int start = body.indexOf("<friendlyName>");
        int end = body.indexOf("</friendlyName>");
        if (start >= 0 && end > start) {
            name = body.substring(start + 14, end);
            name.trim();
        }
    }
    http.end();
    return name;
}

}  // namespace lgremote
