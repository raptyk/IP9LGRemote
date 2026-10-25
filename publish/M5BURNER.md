# Publishing in M5Burner (WebBurner)

M5Launcher's online (OTA) list is fed by the public M5Burner catalogue, so a public M5Burner
entry for the Cardputer also shows up in M5Launcher. M5Burner Desktop is discontinued on
2027-01-01; user firmware is uploaded and managed in **WebBurner**: https://burner.m5stack.com

## Which file to upload

| Field / use | File |
|---|---|
| **M5Burner "FirmWare"** | `release/LGTVRemoteCardputer-<version>-full.bin` – merged image (bootloader + partitions + app), flashed at `0x0` |
| SD card install with M5Launcher | `release/LGTVRemoteCardputer-<version>.bin` – app image only |

Upload the **`-full.bin`**: M5Burner writes images from address `0x0`; M5Launcher extracts the
application from a merged image by itself. Never upload the app-only `.bin` to M5Burner – flashed
at `0x0` it would not boot.

Build both with `pio run`; check the first byte of each file is `0xE9` (ESP image magic).

## Steps

1. Open https://burner.m5stack.com and **Log in** with the M5Stack community account.
2. In your account area add a new firmware (the upload form appears after logging in).
3. Fill in the form from [`description.md`](description.md):
   - **Name**: `LG TV Remote`
   - **Version**: the firmware version, e.g. `1.5.0` (same as `APP_VERSION` in `platformio.ini`)
   - **Description**: the "Description" section
   - **Device type**: Cardputer (and Cardputer ADV if offered)
   - **GitHub link**: https://github.com/raptyk/LGTVRemoteCardputer
   - **FirmWare**: `LGTVRemoteCardputer-<version>-full.bin`
   - **Cover**: [`cover.png`](cover.png) (1280×720) or [`cover-square.png`](cover-square.png) (800×800)
4. After the upload, switch the entry to **public** (Publish button). **Share** creates a share
   code for testers before going public.
5. Test once from WebBurner (Chrome/Edge, USB cable) and once from M5Launcher (OTA list) on a
   real device.

## Updating

Bump `APP_VERSION`, run `pio run`, regenerate the cover if the UI changed
(`python tools/make_cover.py`), add `release-notes-<version>.md`, then edit the existing WebBurner
entry (new version + new `-full.bin`) instead of publishing a second one.

## Starred list in M5Launcher

The "Starred" list in M5Launcher is curated by its author (bmorcelli); ask on the Launcher
Discord / GitHub issues to be considered.
