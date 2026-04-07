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
| 7 | BOARD_CONFIG | Svalboard hardware config (DPI, scroll, automouse) |
| 8 | MBO | Automouse buttons layer with custom MBO modifiers |

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
LED index 0 shows the current machine: white for machine 1, blue for
machine 2. The indicator is split-synced across halves.

#### Trackball swap on KVM_SYS hold

While `KVM_SYS` is held (SYS layer active), the `scroll_hold` flag is set,
which swaps trackball behavior (cursor ↔ scroll) for the duration of the
hold.

#### Thumb Down / DoubleDown grace period

A state-machine in `matrix.c` (`apply_thumb_double_down_grace`) suppresses
the thumb Down switch for a configurable grace window
(`THUMB_DOUBLEDOWN_GRACE_MS`, default 70 ms in `config.h`). If DoubleDown
fires within the window, only DoubleDown is reported. If Down is released
before the window expires (a quick tap), the suppressed press is replayed
via a synthetic hold (`THUMB_TAP_REPLAY_MS`, 10 ms) so the debouncer still
registers it. If the grace window expires and the key is still held, the
state machine enters DOWN_COMMITTED and passes the real key through. If
the user releases within `THUMB_TAP_REPLAY_MS` of entering DOWN_COMMITTED
(before the debouncer can confirm the press), a synthetic replay is used
instead to guarantee the tap registers. This prevents accidental Down
events when the intent is a firm DoubleDown press, while ensuring no taps
are swallowed during rapid typing.

#### MBO modifiers that don't extend automouse timeout

Custom keycodes `SV_MBO_SFT`, `SV_MBO_GUI`, `SV_MBO_ALT`, `SV_MBO_CTL`
on the MBO layer center column. These register the modifier and pin the
automouse layer open (via `mouse_keys_pressed`) without calling
`mouse_mode(true)`, so tapping a modifier during automouse does not reset
the inactivity timeout.

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
held. Used on the MBO layer south keys.

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
- `QUICK_TAP_TERM` 300 ms

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

#### `verify_sync.py`

Checks that the KLE JSON stays in sync with `keymap.c`.

### Other changes

- `keyboards/svalboard/keymaps/keymap_support.h`: added `SV_BOOST_2/3/5`
  to the `my_keycodes` enum
- `keyboards/svalboard/keymaps/keymap_support.c`: added `handle_boost_key`,
  boost enable/disable flags, and wired them into
  `pointing_device_task_combined_user` and `process_record_kb`
- `keyboards/svalboard/matrix.c`: replaced the simple `scans_before_dd_detect`
  logic with the full thumb cluster grace-period state machine
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
python3 keyboards/svalboard/keymaps/glove80_dvorak/render_kle.py \
  --template keyboards/svalboard/keymaps/glove80_dvorak/svalboard-qwerty-layout.json \
  --output keyboards/svalboard/keymaps/glove80_dvorak/glove80_dvorak.kle.json
```

Legends are written as four newline-separated slots per key (BASE, NAV,
NUM, SYM). Tap-hold keys render as `tap (hold)`. Modifier labels use
compact macOS symbols (`⌃`, `⌥`, `⌘`, `⇧`).
