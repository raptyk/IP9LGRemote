# Changelog

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
