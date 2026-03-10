Glove80-derived Dvorak Svalboard keymap

Design goals:
- Preserve Dvorak and the Glove80 macOS home-row mods on the center keys.
- Keep only the layers the user actually uses: base, nav, num, sym, func, sys.
- Use Svalboard automouse instead of dedicated manual mouse layers.
- Shift some punctuation/editing onto the finger wells, following the
  Datahand/Svalboard style instead of forcing a flat-keyboard copy.

Compile examples:
- `make svalboard/trackball/pmw3389/right:glove80_dvorak`
- `make svalboard/trackball/pmw3389/left:glove80_dvorak`
- `make svalboard/trackball/pmw3360/right:glove80_dvorak`
- `make svalboard/trackball/pmw3360/left:glove80_dvorak`
- `make svalboard/right:glove80_dvorak`
- `make svalboard/left:glove80_dvorak`

Workflow:
- Treat `keymap.c` as the source of truth.
- Use Vial/Keybard to experiment live on the board.
- If a Vial change is worth keeping, port it back into `keymap.c`.
- After flashing a new source build, reset the dynamic keymap so the board
  copies the flashed layout back into EEPROM.
- The `.vil` file is kept only as a fallback/archive for stock firmware. It is
  no longer the canonical config and may drift from source over time.

Resetting The Dynamic Keymap:
- Preferred: use the Vial/Keybard layout reset action if the UI exposes it.
  Under the hood this sends VIA's `id_dynamic_keymap_reset`, which copies the
  flashed keymap from firmware into EEPROM and resets the Vial-managed runtime
  settings back to firmware defaults.
- More destructive fallback: bind `EE_CLR` / `QK_CLEAR_EEPROM` temporarily and
  press it once. That invalidates the whole EEPROM and reboots, so the board
  comes back up with firmware defaults, not just the dynamic keymap.
- If you only want your new source layout to become active, prefer the dynamic
  keymap reset over full EEPROM clear.

Notes:
- The source keymap pins the dynamic tap-hold settings that materially affect
  home-row-mod behavior so it stays deterministic even if the board already has
  old Vial settings in EEPROM.
- The source build also adds a Glove80-style app switcher on the otherwise
  unused right thumb `Up` key. It holds `Cmd`, taps `Tab`, and enables a
  temporary typing layer so the home-row mods become plain letters while the
  switcher is open.
- The thumb `Down` / `Pad` / `Nail` keys now mirror the Glove80 base layer's
  `T1` / `T4` / `T5` semantics on both hands. Left `T5` uses the Svalboard
  auto-mouse buttons layer as the trackball-native equivalent of the Glove80
  mouse layer.
- The archival stock `.vil` keeps the source-only app-switch thumb position
  empty because Vial cannot model the same press/hold/release behavior exactly.
