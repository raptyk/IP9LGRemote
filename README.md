# CardputerLGRemote

A "Magic Remote" for LG webOS TVs running on the **M5Stack Cardputer** and **Cardputer ADV**.

- **Infrared** – works with every LG TV out of the box, also while the TV is off.
- **Network (webOS API)** – pointer (cursor), typing on the TV, app launcher, volume level,
  with **LG IP Control** as a TLS-free fallback.
- **Air mouse** – on the Cardputer ADV the BMI270 gyroscope moves the cursor like a Magic Remote;
  on both models the arrow keys can move it too.
- **Bluetooth HID (beta)** – keyboard + mouse + media keys for TVs that accept Bluetooth peripherals.

Author: Patryk Strzelecki <patryk@strzelecki.info> · License: MIT

## Install with M5Launcher

1. Download (or build) `release/CardputerLGRemote-<version>.bin`.
2. Copy it to the SD card (FAT32).
3. In M5Launcher: **SD → select the file → Install**.

The `-full.bin` image (bootloader + partitions + app) can be flashed without Launcher:

```bash
esptool.py --chip esp32s3 write_flash 0x0 release/CardputerLGRemote-1.0.0-full.bin
```

## First steps

1. Point the top edge (IR LED) at the TV and press `p` – that is all IR needs.
2. For the pointer and typing: `opt` → **Wi-Fi** → choose a network → password.
3. `opt` → **TV (network)** → **Search for TVs** → pick yours → accept the prompt on the TV.
   On the TV enable *Settings > General > Devices > TV Management > LG Connect Apps*
   (name varies by model) and, for power-on over Wi-Fi, *Turn on via Wi-Fi*.
4. Optional, webOS 23+: *Settings > Support > IP Control Settings* shows a keycode –
   enter it in **TV (network) > IP Control keycode** for an extra fallback link.

## Keys

| Key | Action | Key | Action |
|---|---|---|---|
| `;` `.` `,` `/` | arrows | `enter` | OK |
| `del` | Back | `esc` | Exit |
| `h` | Home | `s` | Settings |
| `q` | Quick menu | `i` | Input |
| `o` | Info | `e` | Guide |
| `p` | Power | `=` `-` | Volume |
| `m` | Mute | `]` `[` | Channel |
| `0`–`9` | digits | `space` | Play / Pause |
| `z` `x` `c` | Rewind / Stop / Forward | `r` `g` `y` `b` | colour keys |
| `a` / `G0` | pointer on/off | `fn`+`;` `.` | scroll |
| `t` | type on TV | `l` | apps |
| `tab` | link: AUTO → IR → LAN → BLE | `opt` | menu |
| `?` | help | | |

Design rationale: [docs/UX_DESIGN.md](docs/UX_DESIGN.md). Protocol research: [docs/PROTOCOLS.md](docs/PROTOCOLS.md).

## Build

Requires [PlatformIO](https://platformio.org/).

```bash
pio run
```

The post-build script writes both images to `release/`. Flash directly over USB with `pio run -t upload`.

### Project layout

```
src/
  app/        App (main loop, screen stack), Settings (NVS), menus
  core/       RemoteKey, Transport interface, CommandRouter (link selection + fallback)
  ir/         NEC transport and LG code table
  webos/      SSAP client (own FreeRTOS task), manifest, key map, SSDP discovery, Wake-on-LAN
  ipcontrol/  LG IP Control client (AES / PBKDF2)
  ble/        BLE HID composite device (NimBLE)
  hw/         device detection, keyboard events, air mouse
  ui/         theme, strings, painter helpers, screens
```

## Limitations

- The original Magic Remote Bluetooth protocol is not emulated (the TV-side pairing is undocumented,
  see docs/PROTOCOLS.md). The pointer works over the network or as a Bluetooth mouse instead.
- Bluetooth HID support on webOS varies by model – use **Bluetooth > Key test** to check yours.
- No IR receiver on the Cardputer: codes cannot be learned from another remote.
