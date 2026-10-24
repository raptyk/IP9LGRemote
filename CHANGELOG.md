# Changelog

## 1.3.2 – 2026-10-25

- Fixed: the status light still stayed dark. M5Unified's LED driver has no RMT
  implementation for ESP-IDF 4.x (Arduino core 2.x) and silently sends nothing; the LED is now
  driven with the Arduino core RMT driver and NeoPixel timing (like `neopixelWrite()` and the
  MicroPython NeoPixel example), 32-bit frames, brighter flashes.

## 1.3.1 – 2026-10-25

- Fixed: the status light stayed dark on the Cardputer ADV. Its LED is an SK6812 RGBW that
  needs 4-byte frames (M5Unified sends 3); frames are now sent as G-R-B-W over the RMT bus,
  which also works for the 3-byte WS2812 of the original Cardputer. A lit LED is refreshed
  every 250 ms because on the ADV its supply follows the display backlight.

## 1.3.0 – 2026-10-25

- Setup finishes by itself: after the last step (e.g. pairing) the remote opens with a
  "Connected!" pop-up that closes after 2.5 s – no extra "Start remote" click.
- IR mode is ready right away (every LG TV uses the same codes); the IR test is optional
  (home screen, `s` on the IR card).
- Settings start with "About".
- Status light (RGB LED, Settings > Status light, on by default):
  flash in the mode colour = command sent, two red blinks = not delivered,
  slow pulse = connecting / waiting for the TV, two green blinks = connected,
  three red blinks = connection lost, steady glow = pointer mode, red blink every 10 s = battery low.

## 1.2.0 – 2026-10-25

- The active mode is obvious: coloured status bar chip per mode (Wi-Fi blue, IR orange,
  Bluetooth violet, USB green), matching home cards, and the mode icon + name on the remote.
- `tab` opens the help everywhere ("?" needs shift + "/", which is the right arrow).
  `fn` + `;` `.` `,` `/` work as arrows too; in pointer mode `[` `]` scroll.
- Setup checklists grey out later steps; selecting one says which step comes first.
- Service menus: the password (0413, else 0000) is shown, and after In Start / EZ Adjust an
  IR-only keypad opens so digits, arrows and OK reach the TV.
- About starts with the author; enter opens a pop-up with name, e-mail and licence.

## 1.1.1 – 2026-10-25

- Fixed: the Wi-Fi list stayed empty for ~15 s while the Cardputer kept trying to join the
  saved network. Connection attempts pause during a scan, failed scans are retried and the
  connection resumes afterwards.
- The Wi-Fi screen says what is happening ("Searching for Wi-Fi", "Still searching, try 2 of 3")
  and searches again automatically before reporting that nothing was found.

## 1.1.0 – 2026-10-25

Fixes from the first hardware test.

- Fixed: network pairing never connected (WebSocket reconnect interval set before the first attempt).
- Network link alternates port 3001 (TLS) and 3000 until one works; sequential hello →
  getSystemInfo → register handshake; clear failure reasons on the pairing screen.
- PIN pairing (`ssap://pairing/setPin`) next to the "Allow" prompt (key `p` on the pairing screen).
- New home screen with large icons: Wi-Fi remote, IR remote, Bluetooth, USB cable, Settings, Help.
- First-run setup checklist for every mode; the remote opens only when the setup is complete.
- Every menu option explains itself at the bottom of the screen.
- New USB cable mode (USB HID keyboard + mouse) for TVs without Bluetooth keyboard support.
- Word-wrapped texts and font fallback so nothing is cut off on the 240×135 screen
  (`tools/check_text_fit.py` measures all strings with the real font metrics).
- `opt` / `tab` on the remote returns to the home screen.

## 1.0.0 – 2026-10-24

First release.

- IR remote (NEC, LG address 0x04) with the full LG TV key set, discrete power and HDMI codes.
- webOS SSAP client: unsigned manifest, hello + getSystemInfo handshake, pairing screen,
  pointer socket, text input, app launcher, volume feedback, TV keyboard hint, Wake-on-LAN.
- LG IP Control (TCP 9761) fallback link.
- Bluetooth HID composite device (beta) with a key test screen.
- Air mouse on Cardputer ADV (BMI270); arrow-key pointer on both models.
- AUTO / IR / LAN / BLE link modes with per-key fallback to IR.
- Release images for M5Launcher (`app.bin`) and esptool (`-full.bin`).
