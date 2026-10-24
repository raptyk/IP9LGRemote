# Changelog

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
