// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#pragma once

namespace lgremote::webos {

// Registration manifest sent in the SSAP "register" message.
//
// Unsigned on purpose: since 2026 webOS firmware rejects the well-known signed
// "com.lge.test / LG Remote App" manifest with "403 blacklisted certificate"
// (see LGTVCompanion#351, aiowebostv#719). With an unsigned manifest every permission
// is listed in the outer array and the user grants them by accepting the TV prompt.
// Permissions can only be extended by pairing again, so the full set is requested.
// CONTROL_MOUSE_AND_KEYBOARD is required by the pointer socket, CONTROL_INPUT_TEXT by the IME.
constexpr const char* kManifestJson = R"json({
  "manifestVersion": 1,
  "appVersion": "1.1",
  "permissions": [
    "APP_TO_APP", "CLOSE", "CONTROL_AUDIO", "CONTROL_DISPLAY", "CONTROL_INPUT_JOYSTICK",
    "CONTROL_INPUT_MEDIA_PLAYBACK", "CONTROL_INPUT_MEDIA_RECORDING", "CONTROL_INPUT_TEXT",
    "CONTROL_INPUT_TV", "CONTROL_MOUSE_AND_KEYBOARD", "CONTROL_POWER", "CONTROL_TV_SCREEN",
    "LAUNCH", "LAUNCH_WEBAPP", "READ_APP_STATUS", "READ_COUNTRY_INFO", "READ_CURRENT_CHANNEL",
    "READ_INPUT_DEVICE_LIST", "READ_INSTALLED_APPS", "READ_LGE_SDX", "READ_LGE_TV_INPUT_EVENTS",
    "READ_NETWORK_STATE", "READ_NOTIFICATIONS", "READ_POWER_STATE", "READ_RUNNING_APPS",
    "READ_SETTINGS", "READ_TV_CHANNEL_LIST", "READ_TV_CURRENT_TIME", "READ_UPDATE_INFO", "SEARCH",
    "TEST_OPEN", "TEST_PROTECTED", "TEST_SECURE", "UPDATE_FROM_REMOTE_APP",
    "WRITE_NOTIFICATION_ALERT", "WRITE_NOTIFICATION_TOAST", "WRITE_SETTINGS"
  ]
})json";

}  // namespace lgremote::webos
