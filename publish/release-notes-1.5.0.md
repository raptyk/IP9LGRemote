# LG TV Remote 1.5.0

Remote control for LG webOS TVs on the M5Stack Cardputer and Cardputer ADV.

## Highlights

- Four ways to control the TV: **Wi-Fi** (webOS API with pointer and typing), **IR** (every LG
  TV, also when it is off), **Bluetooth** (webOS 5+) and **USB cable**.
- **Air mouse** on the Cardputer ADV (BMI270 gyroscope).
- Home screen with large mode cards and a guided setup checklist for every mode.
- Colour-coded remote screen per mode with a connection badge and on-screen key hints
  (`TAB` help, `OPT` app menu, `A` air mouse).
- Status light: flash on every command, pulse while connecting, green/red for connected/errors.
- **New in 1.5.0:** settings are copied to the SD card (`/config/lgtv-remote/settings.json`)
  and restored automatically after a re-install, so the TV does not need to be paired again.
  Settings > SD card backup can save, restore or delete the copy.

## Notes

- The SD copy contains your Wi-Fi password and the TV pairing key – keep the card private.
- On the Cardputer ADV the status light keeps the screen at full brightness (the LED shares the
  backlight supply); turn Settings > Status light off to dim the screen.
- Wi-Fi pairing: accept the "Allow" prompt on the TV, or press `p` on the pairing screen to pair
  with a PIN.

## Files

| File | Use |
|---|---|
| `LGTVRemoteCardputer-1.5.0-full.bin` | M5Burner upload / esptool at `0x0` (bootloader + partitions + app) |
| `LGTVRemoteCardputer-1.5.0.bin` | M5Launcher from the SD card (app image only) |
