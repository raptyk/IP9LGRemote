# UX design

Design goals for a remote control running on a 240×135 screen with a 56-key keyboard.

## Principles

1. **Eyes on the TV, not on the remote.** Every frequent action is one key press,
   no menus. The screen only confirms what happened.
2. **Mnemonic keys.** A key is chosen by its printed label or its first letter
   (`h` = Home, `m` = Mute, `i` = Input). Physical arrow labels on `;` `,` `.` `/`
   are the D-pad, exactly as printed on the Cardputer keycaps.
3. **One modifier at most.** `Fn` only adds a "secondary" meaning to the same key
   (arrows → scroll). No chords with more than two keys.
4. **Always show where commands go.** The status bar always shows the active
   link (IR / LAN / BLE) and its state, so a "nothing happened" moment is
   explained on screen.
5. **Forgiving.** Destructive or surprising actions (service menu codes, forget
   pairing) need confirmation. Power is a single press because it is the most
   used key, but it never auto-repeats.
6. **Discoverable.** The footer always names the 3 most useful hints; the full
   cheat sheet is one key away (`Fn` + `h` or `?`).

## Screen layout (Remote screen)

```
┌──────────────────────────────────────────┐
│ AUTO ▸ LAN ● Living room TV       ▮▮▮ 87% │  status bar (16 px)
├──────────────────────────────────────────┤
│                                          │
│                 VOL +                    │  last action (large)
│              ▁▂▃▄▅▆▇ 24                   │  optional detail (volume level)
│                                          │
├──────────────────────────────────────────┤
│ tab link · opt menu · ? help             │  footer hints (14 px)
└──────────────────────────────────────────┘
```

- Status bar colour encodes link state: green = ready, amber = connecting,
  red = unavailable (commands fall back to IR in AUTO).
- The action area flashes the accent colour for ~120 ms on every sent command
  (visual "click"), then shows the action name for 1.5 s.
- Pointer mode replaces the action area with a crosshair indicator and the
  hint `enter click · fn+▲▼ scroll · a exit`.

## Key map (Remote screen)

| Key | Action | Notes |
|---|---|---|
| `;` `.` `,` `/` | Up / Down / Left / Right | printed arrows |
| `enter` | OK | |
| `del` | Back | backspace = back, like in a browser |
| `` ` `` (esc) | Exit | |
| `h` | Home | |
| `s` | Settings | |
| `q` | Quick settings / Q.Menu | |
| `i` | Input source | |
| `o` | Info / OSD | |
| `e` | Guide (EPG) | |
| `p` | Power (toggle) | never auto-repeats |
| `=` / `-` | Volume + / − | auto-repeat |
| `m` | Mute | |
| `]` / `[` | Channel + / − | auto-repeat |
| `0`–`9` | Digits | |
| `space` | Play / Pause | toggles |
| `z` `x` `c` | Rewind / Stop / Fast-forward | media cluster on the bottom row |
| `r` `g` `y` `b` | Red / Green / Yellow / Blue | |
| `a` or `G0` button | Pointer mode on/off | air mouse on ADV, arrow keys everywhere |
| `fn` + `;` / `.` | Scroll up / down | pointer and list scrolling |
| `t` | Type text on the TV | LAN or BLE only |
| `l` | App launcher | LAN only |
| `tab` | Next link: AUTO → IR → LAN → BLE | |
| `opt` | Menu (settings) | |
| `?` / `fn`+`h` | Help (cheat sheet) | |

## Links (transports)

- **AUTO** (default): Power goes over IR (works while the TV is off, needs no
  pairing); everything else goes over the network when connected, otherwise IR.
  Works out of the box: with nothing configured it is a plain IR remote.
- **IR**: NEC, address 0x04. Works with every LG TV. No pointer, no text.
- **LAN**: webOS second-screen API (SSAP) – pointer, text, apps, volume level.
  Falls back to LG IP Control (if a keycode is set) and then to IR, key by key.
- **BLE (beta)**: the Cardputer becomes a Bluetooth keyboard + mouse + media remote
  for TVs that accept Bluetooth peripherals. The network link pauses meanwhile
  (RAM). Keys the TV ignores over BLE still go out over IR.

Every command shows which link delivered it ("via IR", "via LAN"), so fallbacks are
never a mystery.

## Apps (l)

Curated list (Netflix, YouTube, Prime Video, Disney+, …). Launched over the network
when connected; Netflix, Prime Video and Disney+ also work over IR (dedicated remote
keys), marked "IR" in the list.

## TV keyboard hint

When a text field on the TV gets focus (SSAP `registerRemoteKeyboard`), a toast
suggests `t` – typing on the Cardputer keyboard is far faster than the on-screen one.

## Pointer mode

Magic Remote users expect "point and click". On the ADV the BMI270 gyroscope
moves the cursor (yaw → X, pitch → Y). On both models the arrow keys move the
cursor with acceleration. `enter` clicks, `del` is still Back, `fn`+arrows
scroll. Pointer mode exits on `a`, `G0`, `esc` or after 30 s of no motion.

## Menu (opt)

Flat list, one level deep, navigated with `;` `.` + `enter`, `del`/`esc` goes back:

1. **Wi-Fi** – scan, pick network, type password.
2. **TV (network)** – search (SSDP) or type IP → pairing screen ("Accept the prompt
   on your TV"), IP Control keycode, Wake-on-LAN MAC (learned automatically),
   pair again, forget.
3. **Bluetooth (beta)** – pairing instructions with live status, key test, forget bonds.
4. **Pointer speed** (1–5) and **Invert pointer Y** – adjusted with `,` `/`.
5. **Brightness**, **Key click sound**.
6. **Service codes (IR)** – Power On/Off, HDMI 1–4, In Start / EZ Adjust (confirmation).
7. **About** – version, device model, motion sensor, IP, free memory, author.

Digits `1`–`9` jump straight to a menu row.

## Text entry (t)

Full-screen single-line field with a blinking caret. Each character is sent to
the TV as you type (LAN: IME `insertText`, BLE: HID keyboard), `del` deletes
one character on the TV, `enter` sends Enter and closes, `esc` closes.

## Feedback & power

- Screen dims after 20 s, turns off after 60 s; any key wakes it (the wake key
  is not sent to the TV, to avoid accidental commands).
- Optional key click sound (off by default).
