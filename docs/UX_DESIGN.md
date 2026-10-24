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

## Home screen (start)

```
┌──────────────────────────────────────────┐
│ LG ● Choose a mode                ▮▮▮ 87% │  status bar (18 px)
├──────────────────────────────────────────┤
│ ◀  (ir)       ┌──────┐        (bt)    ▶  │  neighbours hint at sideways scrolling
│               │ wifi │                   │  40 px icon of the selected card
│               └──────┘                   │
│              Wi-Fi remote                │  title
│        ● Ready: Living room TV           │  green = ready, amber = setup needed
│   Pointer, typing and apps over your     │  two-line explanation
│   home network. Falls back to IR.        │
├──────────────────────────────────────────┤
│ < > choose  enter open  s setup          │  footer hints (16 px)
└──────────────────────────────────────────┘
```

Cards: **Wi-Fi remote**, **IR remote**, **Bluetooth**, **USB cable**, **Settings**, **Help**.
`,` `/` (or `;` `.`) move, `enter`/`G0` opens, `1`–`6` jump, `s` re-runs a mode's setup.
The last used mode is preselected, so the usual start is a single `enter`.

## First-run setup (checklists)

A mode that is not configured opens a checklist instead of the remote. Each row is one
step with its live state on the right (`To do`, `Done`, network name, `Connected`) and
an explanation at the bottom. `enter` performs the step; after each step the cursor
jumps to the next open one. **Start remote** works only when every step is done.

| Mode | Steps |
|---|---|
| Wi-Fi remote | Connect to Wi-Fi → Prepare the TV (LG Connect Apps) → Find the TV (search / IP) → Pair (Allow or PIN) |
| IR remote | Test infrared: sends VOL+ and asks "Did the TV volume go up?" |
| Bluetooth | Check the TV (webOS 5+) → Pair (instructions + live status) |
| USB cable | Connect the cable (USB starts, state turns `Connected` when the TV enumerates it) |

## Screen layout (Remote screen)

```
┌──────────────────────────────────────────┐
│ WI-FI ● Living room TV            ▮▮▮ 87% │  status bar (18 px)
├──────────────────────────────────────────┤
│                                          │
│                 VOL +                    │  last action (18 pt, 12 pt if long)
│              ▁▂▃▄▅▆▇ 24                   │  optional detail (volume level)
│                                          │
├──────────────────────────────────────────┤
│ tab help  opt home  a pointer            │  footer hints (16 px)
└──────────────────────────────────────────┘
```

- Status bar colour encodes link state: green = ready, amber = connecting,
  red = unavailable; in the Wi-Fi mode the text adds "(IR)" while IR is used.
- The action area flashes the accent colour for ~120 ms on every sent command
  (visual "click"), then shows the action name for 1.5 s.
- Pointer mode replaces the action area with a crosshair indicator and the
  hint `enter click · [ ] scroll · a exit`. Arrows accept `fn` too.
- All texts are word-wrapped; `tools/check_text_fit.py` verifies every string with
  the real font metrics (240 px width).

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
| `[` / `]` in pointer mode | Scroll up / down | the Magic Remote wheel |
| `t` | Type text on the TV | Wi-Fi, Bluetooth, USB |
| `l` | App launcher | network; Netflix / Prime / Disney+ also over IR |
| `opt` | Back to the home screen | |
| `tab` (also `?`, `fn`+`h`) | Help (cheat sheet) | `?` = shift + `/`, which is also the right arrow |

## Links (transports)

- **Wi-Fi remote** (`AUTO`): Power goes over IR (works while the TV is off, needs no
  pairing); everything else goes over the webOS API when connected, then LG IP Control
  (if a keycode is set), then IR – key by key.
- **IR**: NEC, address 0x04. Works with every LG TV. No pointer, no text.
- **Bluetooth (beta)**: the Cardputer becomes a Bluetooth keyboard + mouse + media remote
  for TVs with webOS 5+. The network link pauses meanwhile (RAM). Keys the TV ignores over
  BLE still go out over IR.
- **USB cable**: the same HID device over USB – LG TVs accept USB keyboards and mice since
  2012, so this covers TVs without Bluetooth keyboard support.

Every command shows which link delivered it ("via IR", "via WI-FI"), so fallbacks are
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

## Settings (home screen card)

List navigated with `;` `.` + `enter`, `del`/`esc` goes back. The selected row's
explanation is shown at the bottom (two lines), so no option needs a manual:

1. **Wi-Fi** – scan, pick network, type password.
2. **TV (network)** – search / IP address, pairing method (Prompt or PIN), pair again,
   IP Control keycode, Wake-on-LAN MAC (learned automatically), forget.
3. **Bluetooth (beta)** – pairing instructions with live status, key test, forget bonds.
4. **Pointer speed** (1–5) and **Invert pointer Y** – adjusted with `,` `/`.
5. **Brightness**, **Key click sound**.
6. **Service codes (IR)** – Power On/Off, HDMI 1–4, In Start / EZ Adjust (confirmation).
7. **About** – version, device model, motion sensor, IP, free memory, author.

Digits `1`–`9` jump straight to a menu row.

## Pairing screen

One screen for the whole network pairing, with a title and explanation per state:
connecting (with the TV address), "Accept on your TV", "Enter the PIN" (opens a PIN field),
"Pairing failed" / "TV not answering" with the reason and a checklist. `enter` retries,
`p` switches between prompt and PIN pairing, `esc` goes back.

## Text entry (t)

Full-screen single-line field with a blinking caret. Each character is sent to
the TV as you type (LAN: IME `insertText`, BLE: HID keyboard), `del` deletes
one character on the TV, `enter` sends Enter and closes, `esc` closes.

## Feedback & power

- Screen dims after 20 s, turns off after 60 s; any key wakes it (the wake key
  is not sent to the TV, to avoid accidental commands).
- Optional key click sound (off by default).
