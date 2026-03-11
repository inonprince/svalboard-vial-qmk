# Glove80 Migration TODO

Current baseline:
- `keymap.c` is the canonical layout.
- `CHORDAL_HOLD` now defaults to `on` in the source keymap, which restores the same-hand tap-hold suppression that the Glove80 config used.
- Verified builds:
  - `make svalboard/trackball/pmw3389/right:glove80_dvorak`
  - `make svalboard/trackball/pmw3389/left:glove80_dvorak`

High-value imports still available from the Glove80 config:

1. Exact smart selection macros
- Port the Glove80 `select_word`, `extend_word`, `select_line`, and `extend_line` behaviors.
- Current Svalboard nav helpers are only approximations with ordinary shortcuts.
- Source reference:
  - `/Users/inon/repos/zmk-glorious-lefty/config/glove80.keymap` around lines `4115-4180`

2. Thumb combo system
- Port the Glove80 thumb combos that still fit the Svalboard thumb model:
  - sticky shift
  - Caps Word
  - Caps Lock
  - sticky GUI
  - sticky Alt
  - temporary Typing layer access
  - base-layer reset
- This likely requires enabling `COMBO_ENABLE` for the source keymap.
- Source reference:
  - `/Users/inon/repos/zmk-glorious-lefty/config/glove80.keymap` around lines `1511-1688`

3. Additional tab switchers
- Keep the existing source-only `Cmd+Tab` switcher.
- Add the Glove80-style `Ctrl+Tab` and `Gui+Tab` variants with the same temporary `TYPING` layer behavior.
- Source reference:
  - `/Users/inon/repos/zmk-glorious-lefty/config/glove80.keymap` around lines `1541-1580`

4. Per-finger home-row-mod timing
- The Glove80 config uses different hold thresholds by finger:
  - index: `TAPPING_RESOLUTION + 30`
  - middle: `TAPPING_RESOLUTION + 60`
  - ring: `TAPPING_RESOLUTION + 90`
  - pinky: `TAPPING_RESOLUTION + 120`
- Mirror this in QMK with `get_tapping_term()` and `get_quick_tap_term()` instead of a single global value.
- Source reference:
  - `/Users/inon/repos/zmk-glorious-lefty/config/glove80.keymap` around lines `1881-1903`

5. Caps Word continuation behavior
- Match the Glove80 continuation list more closely so Caps Word continues across:
  - `_`
  - `-`
  - `Backspace`
  - `Delete`
  - digits
- Source reference:
  - `/Users/inon/repos/zmk-glorious-lefty/config/glove80.keymap` around lines `4012-4017`

Lower-priority or intentionally skipped imports:
- Gaming layer features
- World / Emoji / Lower layers
- Linux SysRq macro
- Glove80-specific bilateral helper layers
