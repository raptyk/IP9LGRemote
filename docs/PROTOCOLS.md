# LG TV control protocols

Research notes behind the firmware. Status as of October 2026.
**[V]** = verified in source code / documentation, **[I]** = inference, not verified on hardware.

## 1. The Magic Remote itself (Bluetooth LE)

There is **no public specification** of the Magic Remote radio protocol or of the TV-side pairing.
What is known:

- **brainrom/lg-magic** – <https://github.com/brainrom/lg-magic> – Linux HID driver for the MR20.
  The best (and only) protocol source. **[V]**
  - The remote is a standard HID-over-GATT peripheral (`HID_BLUETOOTH_DEVICE(0x000f, 0x3412)`),
    advertising as "LGE MRxx". It pairs with BlueZ like any BLE HID device.
  - Vendor input report **0xFD**, 20 bytes, ~50 Hz:

    | Bytes | Content |
    |---|---|
    | 0 | report id 0xFD |
    | 1–2 | packet counter (LE) |
    | 3–4 | constant |
    | 5–16 | gyro X/Y/Z, accel X/Y/Z – int16 **big-endian** |
    | 17–18 | button code (BE u16) |
    | 19 | wheel delta (int8) |

  - Button code = `0x8000 | <LG NEC IR command>` (HOME 0x807C, BACK 0x8028, OK 0x8044,
    red 0x8072, mic 0x808B…).
  - The remote sends **raw IMU data; the TV computes the pointer** (service `com.webos.service.mrcu`,
    "Magic Remote Control Unit"; see `luna://com.webos.service.mrcu/sensor/getSensorData` used by
    <https://github.com/Wouterdek/magic4pc>).
- TV-side registration (wheel press after boot, Home+Back to re-pair) is undocumented. Whether the
  TV checks the name, PnP ID or performs extra authentication is unknown. **[I]** A native emulation
  would expose the 0xFD report from an ESP32 BLE peripheral; it needs a sniffer capture
  (nRF52840 + Wireshark) of a real pairing first. Not implemented – research stretch goal.
- Older remotes (2010–2013) used Hillcrest Freespace + an RF dongle (likely RF4CE) – irrelevant today.
- LG developer docs (<https://webostv.developer.lge.com/develop/guides/magic-remote>) only describe
  app-level key codes.

## 2. Infrared (NEC)

All LG TVs (and the Magic Remote when Bluetooth is not paired) use **NEC, address 0x04**.
32-bit form: `0x20DF` + bit-reversed command + inverted (e.g. POWER 0x08 → `0x20DF10EF`).
Do not use the "LG 28-bit" protocol – it is for LG air conditioners.

Verified table (sources: Flipper-IRDB `TVs/LG/*`, probonopd/irdb `LG/TV/4,-1.csv`, LG owner's
manual IR table, lg-magic BLE codes):

| Key | Cmd | Key | Cmd |
|---|---|---|---|
| Power toggle | 0x08 | Power ON / OFF (discrete) | 0xC4 / 0xC5 |
| 0–9 | 0x10–0x19 | Vol+ / Vol− / Mute | 0x02 / 0x03 / 0x09 |
| CH+ / CH− | 0x00 / 0x01 | Up / Down / Left / Right | 0x40 / 0x41 / 0x07 / 0x06 |
| OK | 0x44 | Back | 0x28 |
| Exit | 0x5B | Home | 0x7C |
| Settings | 0x43 | Q.Menu | 0x45 |
| Input | 0x0B | Info / Guide | 0xAA / 0xAB |
| Red / Green / Yellow / Blue | 0x72 / 0x71 / 0x63 / 0x61 | Play / Pause / Stop | 0xB0 / 0xBA / 0xB1 |
| Rewind / FF | 0x8F / 0x8E | Record | 0xBD |
| Netflix / Prime Video / Disney+ | 0x56 / 0x5C / 0x31 | HDMI 1/2/3/4 (discrete) | 0xCE / 0xCC / 0xE9 / 0xDA |
| Service: In-Start / EZ-Adjust | 0xFB / 0xFF | | |

Hardware: Cardputer and Cardputer ADV have the IR LED on **GPIO44**; there is no IR receiver.

## 3. webOS second-screen API (SSAP over WebSocket)

Used by the LG ThinQ app, Home Assistant (aiowebostv), PyWebOSTV, LGWebOSRemote, lgtv2.

**Connection** **[V]**
1. `wss://<tv>:3001/` (self-signed certificate, verification disabled); legacy TVs: `ws://<tv>:3000/`.
2. `{"id":"hello","type":"hello","payload":{}}` – required since firmware 33.20 (2025) for the prompt.
3. `ssap://system/getSystemInfo` **before** register (returns 401 after registration on new firmware).
4. `register` with `pairingType:"PROMPT"`, optional stored `client-key` and the manifest.
   Responses: `response` with `pairingType:"PROMPT"` (prompt shown), then `registered` with
   `client-key` (store it), or `error` (declined / rejected).

**Manifest** **[V]** – the well-known *signed* "com.lge.test / LG Remote App" manifest is
**blacklisted** since 2026 (`403 blacklisted certificate`, LGTVCompanion#351, aiowebostv#719).
Use an **unsigned** manifest with all permissions in the outer `permissions` array
(see `src/webos/SsapManifest.h`). Permissions can only be extended by pairing again.

**Requests**: `{"id":"…","type":"request"|"subscribe","uri":"ssap://…","payload":{…}}`.

| Purpose | URI |
|---|---|
| Volume | `ssap://audio/volumeUp`, `volumeDown`, `setMute`, `getVolume` |
| Power | `ssap://system/turnOff`, `com.webos.service.tvpower/power/getPowerState` |
| Media | `ssap://media.controls/play|pause|stop|rewind|fastForward` |
| Apps | `ssap://system.launcher/launch {id}` |
| Inputs | `ssap://tv/switchInput {inputId:"HDMI_1"}` |
| Text (IME) | `ssap://com.webos.service.ime/insertText {text,replace:0}`, `deleteCharacters {count}`, `sendEnterKey`, `registerRemoteKeyboard` (subscribe – text field focus) |
| Toast | `ssap://system.notifications/createToast {message}` |
| Pointer | `ssap://com.webos.service.networkinput/getPointerInputSocket` → `socketPath` |

**Pointer socket** – a second WebSocket to `socketPath`; text frames of `key:value` lines ending
with an empty line:

```
type:move\ndx:5\ndy:-3\ndown:0\n\n
type:click\n\n
type:scroll\ndx:0\ndy:1\n\n
type:button\nname:HOME\n\n
```

Button names: LEFT RIGHT UP DOWN ENTER BACK EXIT HOME MENU QMENU INFO GUIDE LIST DASH RED GREEN
YELLOW BLUE CHANNELUP CHANNELDOWN VOLUMEUP VOLUMEDOWN MUTE PLAY PAUSE STOP REWIND FASTFORWARD
RECORD 0–9 NETFLIX AMAZON …

**Discovery**: SSDP M-SEARCH, ST `urn:lge-com:service:webos-second-screen:1`.
**Wake-on-LAN**: magic packet to UDP 9 (broadcast); requires "Turn on via Wi-Fi" / "Wake on LAN"
on the TV. The TV MAC is learned from the ESP32 ARP table after the first connection.

**ESP32 notes**: one mbedTLS session ≈ 37 KB heap; two sessions (main + pointer) + Wi-Fi + BLE do not
fit together on an ESP32-S3 without PSRAM, so the firmware suspends the network link while BLE is
active. Large responses (`listLaunchPoints`) exceed the 15 KB WebSocket buffer – app IDs are
hard-coded instead.

## 4. LG IP Control (TCP 9761)

Reference: <https://github.com/WesSouza/lgtv-ip-control>. Enabled on webOS 23+ in
*Settings > Support > IP Control Settings*, which shows an 8-character keycode. **[V]**

- Key = PBKDF2-HMAC-SHA256(keycode, salt `63 61 b8 0e 9b dc a6 63 8d 07 20 f2 cc 56 8f b9`,
  16384 iterations, 16 bytes).
- Message = command + `\r`; if already a multiple of 16 append a space; pad PKCS#7-style to 16.
- Packet = AES-128-ECB(random IV) ‖ AES-128-CBC(IV, message). Responses (e.g. `OK`) end with `\n`.
- Commands: `KEY_ACTION <key>`, `POWER off`, `INPUT_SELECT hdmi1`, `APP_LAUNCH <id>`,
  `VOLUME_CONTROL <n>`, `CURRENT_VOL`, `GET_MACADDRESS wifi`…
- Keys: arrowup/down/left/right, ok, returnback, myapp (Home), settingmenu, programminfo,
  programguide, deviceinput, volumeup/down/mute, channelup/down, number0–9, play, rewind,
  fastforward, red/green/yellow/bluebutton…

No TLS, no pairing prompt, unaffected by the SSAP lockdown – but no pointer or text input.
The firmware uses it as fallback of the LAN link.

## 5. Bluetooth HID (generic keyboard / mouse)

LG officially supports Bluetooth keyboards and mice on Bluetooth-equipped models
(*Settings > General > External Devices > Bluetooth Controller*), with a tested-device list.
No public report of an ESP32 BLE HID working with webOS was found, so the firmware marks this
mode **beta** and ships a *Key test* screen (Escape / Backspace / AC Back / AC Home / Menu…) to find
which HID usages the TV maps to Back and Home. **[I]**

## Sources

- brainrom/lg-magic – <https://github.com/brainrom/lg-magic>
- Wouterdek/magic4pc – <https://github.com/Wouterdek/magic4pc>
- andrewfraley/magic_mapper – <https://github.com/andrewfraley/magic_mapper>
- home-assistant-libs/aiowebostv – <https://github.com/home-assistant-libs/aiowebostv>
- supersaiyanmode/PyWebOSTV – <https://github.com/supersaiyanmode/PyWebOSTV>
- klattimer/LGWebOSRemote – <https://github.com/klattimer/LGWebOSRemote>
- hobbyquaker/lgtv2 – <https://github.com/hobbyquaker/lgtv2>
- JPersson77/LGTVCompanion#351 – <https://github.com/JPersson77/LGTVCompanion/issues/351>
- kubrick06010/esp32-webos-remote – <https://github.com/kubrick06010/esp32-webos-remote>
- WesSouza/lgtv-ip-control – <https://github.com/WesSouza/lgtv-ip-control>
- Lucaslhm/Flipper-IRDB – <https://github.com/Lucaslhm/Flipper-IRDB/tree/main/TVs/LG>
- probonopd/irdb – <https://github.com/probonopd/irdb>
- LG: supported keyboard/mouse – <https://www.lg.com/us/support/help-library/supported-keyboard-mouse-peripherals--20154712861019>
- M5Stack Cardputer-Adv – <https://docs.m5stack.com/en/core/Cardputer-Adv>
- bmorcelli/Launcher – <https://github.com/bmorcelli/Launcher>
