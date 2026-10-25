# IP9 LG Remote 1.6.0

Remote control for LG webOS TVs on the M5Stack Cardputer and Cardputer ADV.
**LG TV Remote is now IP9 LG Remote** – one of the IP9 apps for the Cardputer: https://cardputer.ip9.pl

## Highlights

- Same remote as 1.5.0: **Wi-Fi** (webOS API with pointer and typing), **IR** (every LG TV, also
  when it is off), **Bluetooth** (webOS 5+), **USB cable** and the **air mouse** on the Cardputer ADV.
- New name and home: the firmware, the M5Burner entry and the repository are now *IP9 LG Remote*
  (https://github.com/raptyk/IP9LGRemote).
- Built on IP9Kit, the core shared by all IP9 apps – menus, home screen, help, status bar and
  status light work the same in every IP9 app.
- *Settings > About* shows the IP9 website.

## Upgrading

- Settings, Wi-Fi and the TV pairing are kept (NVS and the SD card copy in
  `/config/lgtv-remote/settings.json` are unchanged).
- The Bluetooth name stays "Cardputer Remote", so a paired TV keeps working.

## Files

| File | Use |
|---|---|
| `IP9LGRemote-1.6.0-full.bin` | M5Burner upload / esptool at `0x0` (bootloader + partitions + app) |
| `IP9LGRemote-1.6.0.bin` | M5Launcher from the SD card (app image only) |
