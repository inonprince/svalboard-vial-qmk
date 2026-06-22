# glove80_dvorak — Svalboard keymap

A Dvorak keymap for the Svalboard, ported from a Glove80 layout. This fork
adds the keymap itself plus several firmware-level features on top of the
upstream `svalboard/vial-qmk` repository.

## Changes from upstream

### New keymap: `glove80_dvorak`

An entire custom keymap living under
`keyboards/svalboard/keymaps/glove80_dvorak/`. Everything below is part of
this keymap or supports it.

**Layers** (9 total):

| # | Name | Purpose |
|---|------|---------|
| 0 | BASE | Dvorak alpha with macOS home-row mods (Ctrl/Alt/Gui/Shift on A O E U / H T N S) |
| 1 | NAV | Arrow keys, Home/End/PgUp/PgDn, Find/Undo/Redo, selection macros |
| 2 | NUM | Number pad on the left finger wells, editing shortcuts on the right |
| 3 | SYM | Programmer symbols arranged for easy reach |
| 4 | FUNC | F1-F24, media controls, brightness |
| 5 | SYS | RGB, trackball DPI/scroll config, layer locks |
| 6 | TYPING | Transparent overlay activated during app-switch to unmask home-row mods |
| 7 | BOARD_CONFIG | Reserved transparent layer below the automouse layer index |
| 8 | MBO | Automouse layer, mostly transparent, with mirrored sniper/boost controls |

### Firmware features added

#### App switcher (macOS Cmd+Tab)

Bound to right thumb Up. On press it holds a real `Cmd` modifier and taps
`Tab`, then activates the TYPING overlay so home-row-mod keys send plain
letters (allowing the user to type while the switcher is open). On release,
`Cmd` is unregistered and the overlay is removed. Nested layer-tap holds
(e.g. pressing NAV while the switcher is open) temporarily drop and restore
the TYPING overlay so the target layer becomes visible.

#### KVM switch control with LED indicator

`KVM_SYS` key (left thumb Up): tap sends the `RCtrl RCtrl {1|2}` hotkey
sequence expected by a connected KVM switch and flips the tracked machine.
Hold activates the SYS layer. An RGBLIGHT layer-based LED indicator on
LED index 0 shows the current machine: blue for machine 1, white for
machine 2. The indicator is split-synced across halves and starts blue
on boot.

#### Trackball swap on KVM_SYS hold

While `KVM_SYS` is held (SYS layer active), the `scroll_hold` flag is set,
which swaps trackball behavior (cursor ↔ scroll) for the duration of the
hold. When the roles are swapped, each trackball's report is scaled to match
the other role's saved DPI, so the pointer and scroller keep their usual
sensitivities without waiting for split CPI updates.

#### Manual scroll-role and Mac scroll divisor controls

`SV_SCROLL_TOGGLE` on the SYS layer toggles the same cursor/scroll role swap
that `KVM_SYS` uses temporarily while held. The swap is still DPI-scaled as
described above.

For macOS/iOS hosts, scroll movement can use a high divisor (`MAC_DIVISOR`
120) to match the OS wheel behavior. `SV_MAC_SCROLL_TOGGLE` on SYS toggles
that Mac-only divisor, persists the setting in keyboard EEPROM
(`saved_values` version 7), and defaults it on for upgraded installs. The
status key reports both the stored toggle and whether the divisor is currently
active (`toggle && is_mac`).

#### Thumb Down / DoubleDown grace period

A full-matrix state machine in `matrix.c` filters the debounced matrix after
both halves have been combined. Thumb Down starts pending so a decisive full
press can still resolve to DoubleDown without emitting Down first. If any
non-Down/DoubleDown key is newly pressed while Down is pending, Down is
committed immediately and that new key is delayed by one scan so QMK processes
the layer-tap before the key that depends on it. This works for cross-hand
layer use, e.g. right thumb NUM followed by a left-hand digit.

`THUMB_DOUBLEDOWN_GRACE_MS` is now only the fallback timeout for holding Down
alone with no other key. A quick Down tap is still replayed with
`THUMB_TAP_REPLAY_MS` (10 ms) so QMK sees a normal tap.

#### Mouse click guard

Mouse button keys start a short movement guard
(`SVALBOARD_MOUSE_CLICK_GUARD_MS`, default 50 ms in `config.h`). During
that guard, small pointer movement is discarded so clicking with the other
hand is less likely to turn into a tiny drag. If movement exceeds
`SVALBOARD_MOUSE_CLICK_GUARD_THRESHOLD` (default 12), the guard is canceled
and dragging continues normally.

#### Split pointing report accumulation

Split pointing sync now accumulates remote-half motion until the master
consumes it instead of replacing the shared report with only the latest
packet. The slave polls and accumulates `x/y/h/v` deltas with report clamping,
publishes them with a sequence number, and the master applies each sequence
once before clearing motion after `pointing_device_task` consumes it. This
prevents dropped remote trackball motion when split transport timing is slower
than sensor polling.

#### Manual mouse-click keys with home-row-mod chording

The right-hand R1 and R2 South positions are bound to `KC_BTN1` /
`KC_BTN2` so the trackball can be clicked directly. The MBO/automouse layer is
mostly transparent, so those base-layer click keys continue to pass through
while automouse is active; MBO itself only adds pointer sniper/boost controls.

With the click keys living on the right hand next to home-row-mod keys
(`HM_T`, `HM_S`, ...), naive tap/hold resolution turned fast cmd-click
into a literal `T + click`. Two narrow overrides fix this:

- `get_chordal_hold` returns true when the other key is `KC_BTN1` or
  `KC_BTN2`, so chordal-hold's same-hand rule does not filter the chord
  out as a tap.
- `get_hold_on_other_key_press` returns true only for the eight `HM_*`
  keys, and only while a mouse-button press is flowing through the
  pipeline (tracked via a flag set in `pre_process_record_user` and
  cleared in `process_record_user`). Normal typing keeps its existing
  permissive-hold timing.

To let these keymap-level overrides win at link time, `get_chordal_hold`
and `get_hold_on_other_key_press` are marked `__attribute__((weak))` in
`quantum/qmk_settings.c`. Keymaps that don't override still get Vial's
runtime `QS_tapping_*` settings unchanged.

#### Layer locks

Custom keycodes `SV_LOCK_NAV` through `SV_LOCK_SYS`, `SV_LOCK_MBO`, and
`SV_LOCK_CLEAR` toggle individual layers on/off independent of the
layer-tap hold, using QMK's `layer_lock` feature. Lock keys are placed on
the SYS layer thumbs (and on each layer's own pad key for self-locking).

#### Selection macros

NAV layer left thumb cluster provides one-tap text selection:

| Key | Action |
|-----|--------|
| `SV_SELECT_WORD` | Move to word boundary, select word |
| `SV_EXTEND_WORD` | Extend selection by one word |
| `SV_SELECT_LINE` | Select current line |
| `SV_EXTEND_LINE` | Extend selection by one line |
| `SV_SELECT_NONE` | Deselect (move cursor to collapse selection) |
| `SV_TRIPLE_GRAVE` | Type ` ``` ` (Markdown code fence) |

All four selection/extend macros are **shift-aware**: holding Shift reverses
the direction (e.g. select word to the left instead of right). Modifiers are
saved and restored so held Shift persists across repeated taps.

`SV_EXTEND_WORD` and `SV_EXTEND_LINE` support **hold-to-repeat**: hold the
key and the selection grows continuously (400 ms initial delay, then every
60 ms). The repeat direction is locked at press time. Repeat is cancelled
on layer change.

#### Caps Word continuation

`caps_word_press_user` overrides the QMK default so Caps Word continues
across hyphens (without shifting them to underscore), digits, Backspace,
and Delete. This matches the Glove80 Caps Word behavior.

#### Pointer boost keycodes

New `SV_BOOST_2`, `SV_BOOST_3`, `SV_BOOST_5` keycodes added to
`keymap_support.h/.c` (alongside the existing sniper keys). These multiply
the trackball sensitivity instead of dividing it, giving a speed boost while
held. The current MBO layer mirrors `SV_SNIPER_3` and `SV_BOOST_2` on the
lower South keys of both hands; the other sniper/boost keycodes remain
available as custom Vial keycodes.

#### Svalboard status output

`SV_OUTPUT_STATUS` on the SYS layer types the active keyboard/keymap, full
40-character git hash, dirty/clean state, and `git describe`/`QMK_VERSION`
string. It also reports left/right pointer scroll roles and CPI, axis scroll
lock, the Mac scroll divisor toggle and active state, automouse state, mouse
layer timeout, and turbo scan setting.

The build helper now stores the full git hash in `QMK_GIT_HASH`; the status
formatter strips the dirty marker from the hash itself and reports dirty state
separately.

#### Split startup timing

This keymap raises `SPLIT_USB_TIMEOUT` to 3000 ms and
`SPLIT_WATCHDOG_TIMEOUT` to 10000 ms. That gives RP2040 halves using
`SPLIT_USB_DETECT` more time to decide which side is master and keeps slow
hosts, KVMs, hubs, or a still-starting other half from leaving only one side
alive.

#### Pinned QMK runtime settings

`keyboard_post_init_user` calls `sync_runtime_qmk_settings()` which force-
writes all tap-hold timing parameters to EEPROM on every boot. This ensures
the keymap behaves deterministically regardless of stale Vial/Keybard
settings. Key values:

- `TAPPING_TERM` 225 ms (slightly longer than stock 200 ms for the
  Svalboard's light center keys)
- `PERMISSIVE_HOLD` enabled
- `CHORDAL_HOLD` enabled (rejects same-hand home-row-mod chords)
- `FLOW_TAP_TERM` 60 ms
- default `QUICK_TAP_TERM` 300 ms (preserves tap-then-hold auto-repeat on
  the home-row mods)
- every thumb layer-tap overrides quick-tap to 80 ms so a tap-then-hold
  engages the layer almost immediately instead of repeating the tapped key

### Tools

#### `flash_uf2.py`

A Python script (repo root) for flashing RP2040-based Svalboard halves.
Features:

- Auto-detects the `RPI-RP2` bootloader volume
- If pointed at a directory, picks the newest `.uf2` file
- Auto-detects left/right paired filenames and offers to flash both
  sequentially, waiting for the bootloader drive to disconnect between sides
- Single-keypress confirmation prompts

#### `render_kle.py`

Generates a Keyboard Layout Editor JSON from the C keymap source, producing
per-layer legends with color coding. Reads `keymap.c`, applies human-
friendly label mappings (macOS modifier symbols, shortened names), and
merges results into a KLE template.

The checked-in template and generated sheets use a solid dark `backcolor`
instead of the older KLE ABS background image, and the label map includes
newer custom keycodes such as `SV_MAC_SCROLL_TOGGLE` (`mac div`).

#### `generate_kle.sh`

Convenience wrapper that runs `render_kle.py` twice: once for the primary
cheat sheet (`glove80_dvorak.kle.json`, layers BASE/NAV/NUM/SYM) and once
for a secondary sheet (`glove80_dvorak_secondary.kle.json`,
layers TYPING/FUNC/SYS/MBO).

#### `verify_sync.py`

Checks that the KLE JSON stays in sync with `keymap.c`.

#### `generate_hebrew_keylayout.py`

Generates the macOS Hebrew input layout from the current `BASE` layer and
`hebrew_qwerty_positions.json`. The JSON maps Svalboard physical positions to
the closest classic QWERTY/Hebrew slot; non-text slots such as Esc, Del, and
Win are retained for physical review but skipped. The script derives the
Hebrew output and patches `Hebrew Dvorak.keylayout`.

Preview the generated mapping:

```sh
python3 keyboards/svalboard/keymaps/glove80_dvorak/generate_hebrew_keylayout.py \
  --bundle "/Users/inon/Downloads/Hebrew-Dvorak-22-01-25/Hebrew Dvorak.bundle"
```

Write a copied bundle instead of touching the source bundle:

```sh
python3 keyboards/svalboard/keymaps/glove80_dvorak/generate_hebrew_keylayout.py \
  --bundle "/Users/inon/Downloads/Hebrew-Dvorak-22-01-25/Hebrew Dvorak.bundle" \
  --output-bundle keyboards/svalboard/keymaps/glove80_dvorak/generated/Hebrew\ Dvorak.bundle
```

Pass `--in-place` instead of `--output-bundle` only when you want to rewrite
the source bundle directly.

### Other changes

- `keyboards/svalboard/keymaps/glove80_dvorak/config.h`: added split USB
  detection/watchdog timeout overrides, thumb grace timing, DPI-scaled scroll
  swap, and mouse-click guard config
- `keyboards/svalboard/keymaps/keymap_support.h`: added `SV_BOOST_2/3/5`
  and `SV_MAC_SCROLL_TOGGLE` to the `my_keycodes` enum
- `keyboards/svalboard/keymaps/keymap_support.c`: added boost scaling,
  manual Mac scroll divisor persistence, scroll-role toggle handling, and
  mouse-click guard integration
- `keyboards/svalboard/svalboard.c/.h`: added EEPROM version 7 with
  `mac_scroll_divisor`, expanded `SV_OUTPUT_STATUS`, and full git hash
  reporting
- `keyboards/svalboard/matrix.c`: replaced the simple `scans_before_dd_detect`
  logic with the full thumb cluster Down/DoubleDown interlock
- `quantum/pointing_device/*` and `quantum/split_common/*`: accumulate and
  consume remote split pointing reports so motion is not overwritten between
  master polls
- `keyboards/svalboard/vils/glove80_dvorak.vil`: archived Vial layout

## Compile examples

```sh
make svalboard/trackball/pmw3389/right:glove80_dvorak
make svalboard/trackball/pmw3389/left:glove80_dvorak
make svalboard/trackball/pmw3360/right:glove80_dvorak
make svalboard/trackball/pmw3360/left:glove80_dvorak
make svalboard/right:glove80_dvorak
make svalboard/left:glove80_dvorak
```

## Workflow

- Treat `keymap.c` as the source of truth.
- Use Vial/Keybard to experiment live on the board.
- If a Vial change is worth keeping, port it back into `keymap.c`.
- After flashing a new source build, reset the dynamic keymap so the board
  copies the flashed layout back into EEPROM.
- The `.vil` file is kept only as a fallback/archive for stock firmware.

## Resetting the dynamic keymap

- **Preferred:** use the Vial/Keybard layout reset action. Under the hood
  this sends `id_dynamic_keymap_reset`, which copies the flashed keymap
  from firmware into EEPROM.
- **Destructive fallback:** bind `EE_CLR` / `QK_CLEAR_EEPROM` temporarily
  and press it once. This invalidates the whole EEPROM and reboots.

## KLE export

```sh
keyboards/svalboard/keymaps/glove80_dvorak/generate_kle.sh
```

Renders both cheat sheets in place:
`glove80_dvorak.kle.json` (BASE, NAV, NUM, SYM) and
`glove80_dvorak_secondary.kle.json` (TYPING, FUNC, SYS, MBO).
To render a custom layer set, call `render_kle.py` directly with
`--layers`.

Legends are written as four newline-separated slots per key. Tap-hold
keys render as `tap (hold)`. Modifier labels use compact macOS symbols
(`⌃`, `⌥`, `⌘`, `⇧`).
