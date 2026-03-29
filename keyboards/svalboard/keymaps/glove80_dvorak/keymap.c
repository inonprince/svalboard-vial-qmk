/*
Copyright 2026 Morgan Venable @_claussen

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "../keymap_support.c"
#include "keycodes.h"
#include "quantum_keycodes.h"
#include "layer_lock.h"
#include QMK_KEYBOARD_H
#include <stdbool.h>
#include <stdint.h>
#ifdef QMK_SETTINGS
#include "qmk_settings.h"
#endif
#include "svalboard.h"

/*
 * First-pass Glove80 -> Svalboard conversion:
 * - Base alpha layout stays Dvorak.
 * - Center keys keep the Glove80-style macOS home-row mods.
 * - Only the layers the user actually uses are kept.
 * - Trackball uses Svalboard's built-in automouse layer instead of dedicated
 *   manual mouse layers.
 */

#define HM_A LCTL_T(KC_A)
#define HM_O LALT_T(KC_O)
#define HM_E LGUI_T(KC_E)
#define HM_U LSFT_T(KC_U)
#define HM_H LSFT_T(KC_H)
#define HM_T LGUI_T(KC_T)
#define HM_N LALT_T(KC_N)
#define HM_S LCTL_T(KC_S)

#define MAC_UNDO      LGUI(KC_Z)
#define MAC_REDO      SGUI(KC_Z)
#define MAC_CUT       LGUI(KC_X)
#define MAC_COPY      LGUI(KC_C)
#define MAC_PASTE     LGUI(KC_V)
#define MAC_FIND      LGUI(KC_F)
#define MAC_FIND_NEXT LGUI(KC_G)
#define MAC_FIND_PREV SGUI(KC_G)
#define MAC_SAVE      LGUI(KC_S)
#define MAC_SELECT_ALL LGUI(KC_A)

#define WORD_LEFT      LALT(KC_LEFT)
#define WORD_RIGHT     LALT(KC_RIGHT)
#define LINE_START     LGUI(KC_LEFT)
#define LINE_END       LGUI(KC_RIGHT)
#define SEL_WORD_LEFT  LSA(KC_LEFT)
#define SEL_WORD_RIGHT LSA(KC_RIGHT)
#define SEL_LINE_START SGUI(KC_LEFT)
#define SEL_LINE_END   SGUI(KC_RIGHT)
#define SEL_DOWN       S(KC_DOWN)

#define TILD S(KC_GRV)
#define PIPE S(KC_BSLS)
#define DQUO S(KC_QUOT)
#define COLN S(KC_SCLN)
#define EXLM S(KC_1)
#define AT   S(KC_2)
#define HASH S(KC_3)
#define DLR  S(KC_4)
#define PERC S(KC_5)
#define CIRC S(KC_6)
#define AMPR S(KC_7)
#define ASTR S(KC_8)
#define LPRN S(KC_9)
#define RPRN S(KC_0)
#define UNDS S(KC_MINS)
#define PLUS S(KC_EQL)
#define LTGT S(KC_COMM)
#define GTGT S(KC_DOT)
#define QUES S(KC_SLSH)
#define LCBR S(KC_LBRC)
#define RCBR S(KC_RBRC)

static bool kvm_next_is_two = false;

/* RGBLIGHT_LAYERS: KVM indicator on left LED (index 0) only.
 * Lighting layer 0 = machine 1 (white), layer 1 = machine 2 (blue).
 * The enabled_layer_mask is split-synced, so the slave applies the
 * override inside rgblight_set() → rgblight_layers_write(). */
const rgblight_segment_t PROGMEM kvm_one_seg[] = RGBLIGHT_LAYER_SEGMENTS({0, 1, 0x00, 0x00, 0xFF});
const rgblight_segment_t PROGMEM kvm_two_seg[] = RGBLIGHT_LAYER_SEGMENTS({0, 1, 0xAA, 0xFF, 0xFF});

const rgblight_segment_t * const PROGMEM kvm_rgb_layers[] = RGBLIGHT_LAYERS_LIST(
    kvm_one_seg,
    kvm_two_seg
);

static void update_layer_indicator(uint32_t layer, bool save) {
  if (layer > 15) {
    layer = 15;
  }

  /* Toggle KVM lighting layers before setting base color, so
   * rgblight_layers_write() inside rgblight_set() sees the new mask. */
  rgblight_set_layer_state(0, !kvm_next_is_two);
  rgblight_set_layer_state(1, kvm_next_is_two);

  /* Base color on both LEDs; rgblight_layers_write() then overrides LED 0. */
  sval_set_active_layer(layer, save);
}

layer_state_t default_layer_state_set_user(layer_state_t state) {
  update_layer_indicator(0, false);
  return state;
}

layer_state_t layer_state_set_user(layer_state_t state) {
  update_layer_indicator(get_highest_layer(state), false);
  return state;
}

enum layer {
    BASE,
    NAV,
    NUM,
    SYM,
    FUNC,
    SYS,
    TYPING,
    BOARD_CONFIG = MH_AUTO_BUTTONS_LAYER - 1,
    MBO = MH_AUTO_BUTTONS_LAYER,
};

/* Thumb layer-taps: tap sends the key, hold activates the layer. */
#define TH_NUM  LT(NUM, KC_DEL)
#define TH_NAV  LT(NAV, KC_SPACE)
#define TH_FUNC LT(FUNC, KC_ENTER)
#define TH_MBO  LT(MBO, KC_TAB)
#define TH_SYM  LT(SYM, KC_BSPC)
#define TH_SYS  LT(SYS, KC_ESC)
/* Tap toggles KVM; hold activates SYS layer. KC_NO tap is intercepted. */
#define KVM_SYS LT(SYS, KC_NO)

enum custom_keycodes {
    SV_APP_SWITCH = QK_KB_20,
    SV_SELECT_NONE,
    SV_SELECT_WORD,
    SV_EXTEND_WORD,
    SV_SELECT_LINE,
    SV_EXTEND_LINE,
    SV_TRIPLE_GRAVE,
    SV_MBO_SFT,
    SV_MBO_GUI,
    SV_MBO_ALT,
    SV_MBO_CTL,
    SV_LOCK_NAV,
    SV_LOCK_NUM,
    SV_LOCK_SYM,
    SV_LOCK_FUNC,
    SV_LOCK_SYS,
    SV_LOCK_MBO,
    SV_LOCK_CLEAR,
};

static bool app_switch_active = false;
static bool app_switch_added_gui = false;
/* Track nested layer-tap holds so TYPING overlay is only removed/restored
 * at the outermost boundary during app switching. */
static uint8_t app_switch_layer_tap_depth = 0;
/* Bitmask of mods currently pinning the automouse layer open on MBO. */
static uint8_t mbo_mouse_layer_mods = 0;

static bool is_app_switch_layer_tap(uint16_t keycode) {
  return IS_QK_LAYER_TAP(keycode);
}

/* Send the RCTL-RCTL-{1|2} sequence expected by the KVM switch,
 * then flip the tracked machine and update the LED indicator. */
static void trigger_kvm_switch(void) {
  tap_code(KC_RCTL);
  tap_code(KC_RCTL);
  tap_code(kvm_next_is_two ? KC_2 : KC_1);
  kvm_next_is_two = !kvm_next_is_two;
  update_layer_indicator(get_highest_layer(layer_state), false);
}

/* tap_code16 + 1ms pause so macOS registers multi-key selection macros. */
static void tap_code16_wait(uint16_t keycode) {
  tap_code16(keycode);
  wait_ms(1);
}

/* MBO modifier that pins the automouse layer while held but does NOT reset
 * the timeout on tap.  Plain KC_L* mods would call mouse_mode(true) via
 * keymap_support.c on every press, extending the timeout even for taps. */
static bool handle_mouse_layer_mod(keyrecord_t *record, uint8_t mod_bit) {
  if (record->event.pressed) {
    register_mods(mod_bit);
    if (!(mbo_mouse_layer_mods & mod_bit) && (layer_state & (1 << MH_AUTO_BUTTONS_LAYER))) {
      mouse_keys_pressed++;
      mbo_mouse_layer_mods |= mod_bit;
    }
  } else {
    unregister_mods(mod_bit);
    if (mbo_mouse_layer_mods & mod_bit) {
      if (mouse_keys_pressed > 0) {
        mouse_keys_pressed--;
      }
      mbo_mouse_layer_mods &= ~mod_bit;
    }
  }

  return false;
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
  if (app_switch_active && app_switch_added_gui && keycode != SV_APP_SWITCH &&
      record->event.pressed) {
    register_weak_mods(MOD_BIT(KC_LGUI));
  }

  /* While the app switcher is open, layer-tap keys (e.g. TH_NAV) would
   * activate layers under the TYPING overlay. Temporarily drop TYPING
   * so the target layer becomes visible, and restore it on release. */
  if (app_switch_active && keycode != SV_APP_SWITCH &&
      is_app_switch_layer_tap(keycode)) {
    if (record->event.pressed) {
      if (app_switch_layer_tap_depth++ == 0) {
        layer_off(TYPING);
      }
    } else if (app_switch_layer_tap_depth > 0) {
      if (--app_switch_layer_tap_depth == 0) {
        layer_on(TYPING);
      }
    }
  }

  switch (keycode) {
    case SV_APP_SWITCH:
      if (record->event.pressed) {
        if (!app_switch_active) {
          uint8_t mods = get_mods() | get_weak_mods();

          // Hold a plain-typing overlay and Cmd while the switcher stays open.
          layer_on(TYPING);
          app_switch_added_gui = (mods & MOD_MASK_GUI) == 0;
          if (app_switch_added_gui) {
            register_weak_mods(MOD_BIT(KC_LGUI));
          }
          tap_code(KC_TAB);
          app_switch_active = true;
        }
      } else if (app_switch_active) {
        if (app_switch_added_gui) {
          unregister_weak_mods(MOD_BIT(KC_LGUI));
        }
        layer_off(TYPING);
        app_switch_layer_tap_depth = 0;
        app_switch_added_gui = false;
        app_switch_active = false;
      }
      return false;

    /* Selection macros: move cursor to deselect, select word/line, or
     * extend an existing selection by one word/line. */
    case SV_SELECT_NONE:
      if (record->event.pressed) {
        tap_code16_wait(KC_DOWN);
        tap_code16_wait(KC_UP);
        tap_code16_wait(KC_RIGHT);
        tap_code16(KC_LEFT);
      }
      return false;

    case SV_SELECT_WORD:
      if (record->event.pressed) {
        tap_code16_wait(WORD_RIGHT);
        tap_code16_wait(WORD_LEFT);
        tap_code16(SEL_WORD_RIGHT);
      }
      return false;

    case SV_EXTEND_WORD:
      if (record->event.pressed) {
        tap_code16(SEL_WORD_RIGHT);
      }
      return false;

    case SV_SELECT_LINE:
      if (record->event.pressed) {
        tap_code16_wait(LINE_START);
        tap_code16(SEL_LINE_END);
      }
      return false;

    case SV_EXTEND_LINE:
      if (record->event.pressed) {
        tap_code16_wait(SEL_DOWN);
        tap_code16(SEL_LINE_END);
      }
      return false;

    case SV_TRIPLE_GRAVE:
      if (record->event.pressed) {
        SEND_STRING("```");
      }
      return false;

    case SV_MBO_SFT:
      return handle_mouse_layer_mod(record, MOD_BIT(KC_LSFT));

    case SV_MBO_GUI:
      return handle_mouse_layer_mod(record, MOD_BIT(KC_LGUI));

    case SV_MBO_ALT:
      return handle_mouse_layer_mod(record, MOD_BIT(KC_LALT));

    case SV_MBO_CTL:
      return handle_mouse_layer_mod(record, MOD_BIT(KC_LCTL));

    /* Layer locks: toggle a layer on/off independent of the layer-tap. */
    case SV_LOCK_NAV:
      if (record->event.pressed) {
        layer_lock_invert(NAV);
      }
      return false;

    case SV_LOCK_NUM:
      if (record->event.pressed) {
        layer_lock_invert(NUM);
      }
      return false;

    case SV_LOCK_SYM:
      if (record->event.pressed) {
        layer_lock_invert(SYM);
      }
      return false;

    case SV_LOCK_FUNC:
      if (record->event.pressed) {
        layer_lock_invert(FUNC);
      }
      return false;

    case SV_LOCK_SYS:
      if (record->event.pressed) {
        layer_lock_invert(SYS);
      }
      return false;

    case SV_LOCK_MBO:
      if (record->event.pressed) {
        layer_lock_invert(MBO);
      }
      return false;

    case SV_LOCK_CLEAR:
      if (record->event.pressed) {
        layer_lock_all_off();
      }
      return false;

    case KVM_SYS: /* tap = KVM switch, hold = SYS layer (handled by LT) */
      if (!record->event.pressed && record->tap.count) {
        trigger_kvm_switch();
        return false;
      }
      return true;
  }

  return true;
}

#ifdef QMK_SETTINGS
enum qmk_setting_id {
    QSID_GRAVE_ESC_OVERRIDE = 1,
    QSID_COMBO_TERM = 2,
    QSID_AUTO_SHIFT = 3,
    QSID_AUTO_SHIFT_TIMEOUT = 4,
    QSID_ONESHOT_TAP_TOGGLE = 5,
    QSID_ONESHOT_TIMEOUT = 6,
    QSID_TAPPING_TERM = 7,
    QSID_MOUSEKEY_DELAY = 9,
    QSID_MOUSEKEY_INTERVAL = 10,
    QSID_MOUSEKEY_MOVE_DELTA = 11,
    QSID_MOUSEKEY_MAX_SPEED = 12,
    QSID_MOUSEKEY_TIME_TO_MAX = 13,
    QSID_MOUSEKEY_WHEEL_DELAY = 14,
    QSID_MOUSEKEY_WHEEL_INTERVAL = 15,
    QSID_MOUSEKEY_WHEEL_MAX_SPEED = 16,
    QSID_MOUSEKEY_WHEEL_TIME_TO_MAX = 17,
    QSID_TAP_CODE_DELAY = 18,
    QSID_TAP_HOLD_CAPS_DELAY = 19,
    QSID_TAPPING_TOGGLE = 20,
    QSID_MAGIC = 21,
    QSID_PERMISSIVE_HOLD = 22,
    QSID_HOLD_ON_OTHER_KEY_PRESS = 23,
    QSID_RETRO_TAPPING = 24,
    QSID_QUICK_TAP_TERM = 25,
    QSID_CHORDAL_HOLD = 26,
    QSID_FLOW_TAP_TERM = 27,
};

static void set_qmk_setting_u8(uint16_t qsid, uint8_t value) {
  qmk_settings_set(qsid, &value, sizeof(value));
}

static void set_qmk_setting_u16(uint16_t qsid, uint16_t value) {
  qmk_settings_set(qsid, &value, sizeof(value));
}

static void set_qmk_setting_u32(uint16_t qsid, uint32_t value) {
  qmk_settings_set(qsid, &value, sizeof(value));
}

/*
 * Pin the runtime settings so the source-built keymap behaves deterministically
 * regardless of existing EEPROM state. This approximates the Glove80 bilateral
 * home-row-mod behavior with QMK's closest knobs:
 * - a moderate global tapping term for the Svalboard's light switches
 * - PERMISSIVE_HOLD to mimic ZMK's hold-trigger-on-release behavior
 * - CHORDAL_HOLD to reject same-hand HRM chords
 * - FLOW_TAP_TERM to approximate ZMK's require-prior-idle streak decay
 * - a longer QUICK_TAP_TERM to preserve tap-then-hold repeat behavior
 */
static void sync_runtime_qmk_settings(void) {
  set_qmk_setting_u8(QSID_GRAVE_ESC_OVERRIDE, 0);
  set_qmk_setting_u16(QSID_COMBO_TERM, 50);
  set_qmk_setting_u8(QSID_AUTO_SHIFT, 0);
  set_qmk_setting_u16(QSID_AUTO_SHIFT_TIMEOUT, 175);
  set_qmk_setting_u8(QSID_ONESHOT_TAP_TOGGLE, 5);
  set_qmk_setting_u16(QSID_ONESHOT_TIMEOUT, 5000);
  set_qmk_setting_u16(QSID_TAPPING_TERM, TAPPING_TERM);
  set_qmk_setting_u16(QSID_MOUSEKEY_DELAY, 30);
  set_qmk_setting_u16(QSID_MOUSEKEY_INTERVAL, 20);
  set_qmk_setting_u16(QSID_MOUSEKEY_MOVE_DELTA, 8);
  set_qmk_setting_u16(QSID_MOUSEKEY_MAX_SPEED, 15);
  set_qmk_setting_u16(QSID_MOUSEKEY_TIME_TO_MAX, 40);
  set_qmk_setting_u16(QSID_MOUSEKEY_WHEEL_DELAY, 10);
  set_qmk_setting_u16(QSID_MOUSEKEY_WHEEL_INTERVAL, 80);
  set_qmk_setting_u16(QSID_MOUSEKEY_WHEEL_MAX_SPEED, 8);
  set_qmk_setting_u16(QSID_MOUSEKEY_WHEEL_TIME_TO_MAX, 40);
  set_qmk_setting_u16(QSID_TAP_CODE_DELAY, 0);
  set_qmk_setting_u16(QSID_TAP_HOLD_CAPS_DELAY, 80);
  set_qmk_setting_u8(QSID_TAPPING_TOGGLE, 5);
  set_qmk_setting_u32(QSID_MAGIC, 128);
  set_qmk_setting_u8(QSID_PERMISSIVE_HOLD, 1);
  set_qmk_setting_u8(QSID_HOLD_ON_OTHER_KEY_PRESS, 0);
  set_qmk_setting_u8(QSID_RETRO_TAPPING, 0);
  set_qmk_setting_u16(QSID_QUICK_TAP_TERM, 300);
  set_qmk_setting_u8(QSID_CHORDAL_HOLD, 1);
  set_qmk_setting_u16(QSID_FLOW_TAP_TERM, 80);
}
#endif

#if __has_include("keymap_all.h")
#include "keymap_all.h"
#else
int sval_macro_size = 0;
uint8_t sval_macros[] = {0};

const uint16_t PROGMEM keymaps[DYNAMIC_KEYMAP_LAYER_COUNT][MATRIX_ROWS][MATRIX_COLS] = {
    [BASE] = LAYOUT(
        /*     Center            North            East              South             West             Double */
        /*R1*/ HM_H            , KC_G           , KC_F            , KC_M            , KC_D          , KC_NO ,
        /*R2*/ HM_T            , KC_C           , KC_RBRC         , KC_W            , KC_LBRC       , KC_NO ,
        /*R3*/ HM_N            , KC_R           , KC_EQL          , KC_V            , KC_B          , KC_NO ,
        /*R4*/ HM_S            , KC_L           , KC_MINS         , KC_Z            , QK_REPEAT_KEY , KC_NO ,
        /*L1*/ HM_U            , KC_P           , KC_I            , KC_K            , KC_Y          , KC_NO ,
        /*L2*/ HM_E            , KC_DOT         , S(KC_TAB)       , KC_J            , KC_GRV        , KC_NO ,
        /*L3*/ HM_O            , KC_COMM        , KC_X            , KC_Q            , KC_ESC        , KC_NO ,
        /*L4*/ HM_A            , KC_QUOT        , KC_BSLS         , KC_SCLN         , KC_DEL        , KC_NO ,

        /*     Down               Pad                 Up              Nail               Knuckle          DoubleDown */
        /*RT*/ TH_NUM          , TH_NAV          , KVM_SYS       , TH_FUNC           , LCAG(KC_NO)     , KC_CAPS ,
        /*LT*/ TH_MBO          , TH_SYM          , SV_APP_SWITCH , TH_SYS            , LCAG(KC_NO)     , SV_CAPS_WORD
    ),

    [NAV] = LAYOUT(
        /*     Center            North             East               South              West              Double */
        /*R1*/ KC_LSFT         , S(KC_TAB)       , KC_HOME          , KC_TAB            , KC_LEFT         , KC_NO ,
        /*R2*/ KC_LGUI         , KC_DEL          , KC_PGDN          , KC_BSPC           , KC_DOWN         , KC_NO ,
        /*R3*/ KC_LALT         , KC_INS          , KC_PGUP          , KC_SPACE          , KC_UP           , KC_NO ,
        /*R4*/ KC_LCTL         , KC_ESC          , KC_END           , KC_ENTER          , KC_RIGHT        , KC_NO ,
        /*L1*/ KC_RIGHT        , S(KC_TAB)       , MAC_FIND         , KC_F16            , KC_TRNS         , KC_NO ,
        /*L2*/ KC_UP           , MAC_REDO        , MAC_FIND_NEXT    , KC_F17            , KC_TRNS         , KC_NO ,
        /*L3*/ KC_DOWN         , MAC_UNDO        , MAC_FIND_PREV    , KC_F18            , KC_TRNS         , KC_NO ,
        /*L4*/ KC_LEFT         , KC_TAB          , KC_TRNS          , KC_F19            , OSM(MOD_LSFT)   , KC_NO ,

        /*     Down            Pad      Up       Nail     Knuckle  DoubleDown */
        /*RT*/ KC_TRNS       , SV_LOCK_NAV   , KC_TRNS         , KC_TRNS         , KC_TRNS        , KC_TRNS ,
        /*LT*/ SV_SELECT_LINE, SV_SELECT_WORD, MAC_SELECT_ALL , SV_EXTEND_WORD  , SV_EXTEND_LINE , SV_SELECT_NONE
    ),

    [NUM] = LAYOUT(
        /*     Center            North             East              South             West              Double */
        /*R1*/ KC_LSFT         , S(KC_TAB)       , KC_TAB          , MAC_UNDO         , MAC_FIND        , KC_NO ,
        /*R2*/ KC_LGUI         , KC_DEL          , KC_BSPC         , MAC_REDO         , SEL_WORD_RIGHT  , KC_NO ,
        /*R3*/ KC_LALT         , KC_INS          , KC_SPACE        , MAC_FIND_PREV    , SEL_LINE_END    , KC_NO ,
        /*R4*/ KC_LCTL         , KC_ESC          , KC_ENTER        , MAC_FIND_NEXT    , MAC_SELECT_ALL  , KC_NO ,
        /*L1*/ KC_6            , KC_9            , DLR             , KC_3             , TILD            , KC_NO ,
        /*L2*/ KC_5            , KC_8            , COLN            , KC_2             , PERC            , KC_NO ,
        /*L3*/ KC_4            , KC_7            , RPRN            , KC_1             , LPRN            , KC_NO ,
        /*L4*/ PLUS            , EXLM            , ASTR            , KC_MINS          , KC_SLSH         , KC_NO ,

        /*     Down            Pad      Up       Nail     Knuckle  DoubleDown */
        /*RT*/ SV_LOCK_NUM   , KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS ,
        /*LT*/ KC_DOT        , KC_0   , KC_EQL , KC_COMM, LTGT   , GTGT
    ),

    [SYM] = LAYOUT(
        /*     Center            North             East              South             West              Double */
        /*R1*/ RPRN            , KC_LBRC         , QUES            , PLUS             , LPRN            , KC_NO ,
        /*R2*/ LCBR            , KC_RBRC         , PIPE            , ASTR             , DQUO            , KC_NO ,
        /*R3*/ RCBR            , LTGT            , TILD            , KC_SLSH          , KC_MINS         , KC_NO ,
        /*R4*/ KC_EQL          , GTGT            , CIRC            , UNDS             , KC_SCLN         , KC_NO ,
        /*L1*/ KC_TAB          , S(KC_TAB)       , KC_QUOT         , EXLM             , KC_LSFT         , KC_NO ,
        /*L2*/ KC_ENTER        , KC_DEL          , KC_GRV          , AMPR             , KC_LGUI         , KC_NO ,
        /*L3*/ KC_SPACE        , KC_INS          , SV_TRIPLE_GRAVE , HASH             , KC_LALT         , KC_NO ,
        /*L4*/ KC_ESC          , KC_BSPC         , KC_TRNS         , DLR              , KC_TRNS         , KC_NO ,

        /*     Down            Pad      Up       Nail     Knuckle  DoubleDown */
        /*RT*/ COLN          , PERC   , AT     , KC_BSLS, KC_DOT , ASTR ,
        /*LT*/ KC_TRNS       , SV_LOCK_SYM, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    ),

    [FUNC] = LAYOUT(
        /*     Center            North              East              South             West               Double */
        /*R1*/ KC_F9           , KC_MPRV          , KC_MNXT         , KC_F10          , KC_MPLY          , KC_NO ,
        /*R2*/ KC_F11          , KC_VOLU          , KC_MUTE         , KC_F12          , KC_VOLD          , KC_NO ,
        /*R3*/ KC_PSCR         , KC_BRIU          , KC_NO           , KC_PAUSE        , KC_BRID          , KC_NO ,
        /*R4*/ KC_APP          , KC_EJCT          , KC_CALC         , KC_WHOM         , KC_NO            , KC_NO ,
        /*L1*/ KC_F7           , KC_F8            , KC_F9           , KC_F10          , KC_F24           , KC_NO ,
        /*L2*/ KC_F5           , KC_F6            , KC_F11          , KC_F12          , KC_F23           , KC_NO ,
        /*L3*/ KC_F3           , KC_F4            , KC_F13          , KC_F14          , KC_F22           , KC_NO ,
        /*L4*/ KC_F1           , KC_F2            , KC_F15          , KC_F16          , KC_F21           , KC_NO ,

        /*     Down            Pad      Up       Nail     Knuckle  DoubleDown */
        /*RT*/ KC_TRNS       , KC_TRNS, KC_TRNS, SV_LOCK_FUNC, KC_TRNS, KC_TRNS ,
        /*LT*/ KC_VOLD       , KC_MUTE, KC_VOLU, KC_MPRV, KC_MNXT, KC_MPLY
    ),

    [SYS] = LAYOUT(
        /*     Center                North                  East                 South                 West                 Double */
        /*R1*/ RGB_TOG             , RGB_MOD              , RGB_HUI            , RGB_HUD            , RGB_SAI            , KC_NO ,
        /*R2*/ RGB_VAI             , RGB_SPD              , RGB_VAD            , RGB_SAD            , RGB_SPI            , KC_NO ,
        /*R3*/ KC_PAUSE            , KC_PSCR              , KC_APP             , KC_CAPS            , KC_NUM             , KC_NO ,
        /*R4*/ KC_SCRL             , KC_NO                , KC_NO              , KC_NO              , KC_NO              , KC_NO ,
        /*L1*/ SV_OUTPUT_STATUS    , SV_MH_CHANGE_TIMEOUTS, SV_TOGGLE_AUTOMOUSE, SV_AXIS_SCROLL_LOCK, SV_CAPS_WORD       , KC_NO ,
        /*L2*/ SV_RIGHT_DPI_INC    , SV_RIGHT_SCROLL_TOGGLE, SV_SNIPER_2       , SV_RIGHT_DPI_DEC   , KC_NO              , KC_NO ,
        /*L3*/ SV_LEFT_DPI_INC     , SV_LEFT_SCROLL_TOGGLE, SV_SNIPER_3        , SV_LEFT_DPI_DEC    , KC_NO              , KC_NO ,
        /*L4*/ SV_SCROLL_HOLD      , SV_SCROLL_TOGGLE     , KC_NO              , KC_NO              , KC_NO              , KC_NO ,

        /*     Down            Pad             Up       Nail            Knuckle          DoubleDown */
        /*RT*/ SV_LOCK_NUM    , SV_LOCK_NAV    , KC_TRNS, SV_LOCK_FUNC  , SV_LOCK_CLEAR   , KC_TRNS ,
        /*LT*/ SV_LOCK_MBO    , SV_LOCK_SYM    , KC_TRNS, SV_LOCK_SYS   , SV_LOCK_CLEAR   , KC_TRNS
    ),

    /* TYPING: overlay activated during app-switch to unmask home-row mods
     * so Cmd+Tab/arrow work. Center keys emit plain letters; most others
     * are transparent so the base layer still handles them. */
    [TYPING] = LAYOUT(
        /*     Center            North    East     South    West     Double */
        /*R1*/ KC_H            , KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_NO ,
        /*R2*/ KC_T            , KC_TRNS, KC_UP  , KC_TRNS, KC_DOWN, KC_NO ,
        /*R3*/ KC_N            , KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_NO ,
        /*R4*/ KC_S            , KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_NO ,
        /*L1*/ KC_U            , KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_NO ,
        /*L2*/ KC_E            , KC_TRNS, KC_DOWN, KC_TRNS, KC_UP  , KC_NO ,
        /*L3*/ KC_O            , KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_NO ,
        /*L4*/ KC_A            , KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_NO ,

        /*     Down            Pad      Up             Nail     Knuckle  DoubleDown */
        /*RT*/ KC_TRNS       , KC_TRNS, KC_TRNS       , KC_TRNS, KC_TRNS, KC_TRNS ,
        /*LT*/ KC_TRNS       , KC_TRNS, SV_APP_SWITCH , KC_TRNS, KC_TRNS, KC_TRNS
    ),

    [BOARD_CONFIG] = LAYOUT(
        /*     Center                North                  East                  South                 West                Double */
        /*R1*/ KC_TRNS             , KC_TRNS              , KC_TRNS             , KC_TRNS             , KC_TRNS            , KC_NO ,
        /*R2*/ KC_TRNS             , KC_TRNS              , KC_TRNS             , KC_TRNS             , KC_TRNS            , KC_NO ,
        /*R3*/ KC_TRNS             , KC_TRNS              , KC_TRNS             , KC_TRNS             , KC_TRNS            , KC_NO ,
        /*R4*/ KC_TRNS             , KC_TRNS              , KC_TRNS             , KC_TRNS             , KC_TRNS            , KC_NO ,
        /*L1*/ SV_OUTPUT_STATUS    , SV_MH_CHANGE_TIMEOUTS, SV_TOGGLE_AUTOMOUSE , KC_TRNS             , SV_CAPS_WORD       , KC_NO ,
        /*L2*/ SV_RIGHT_DPI_INC    , SV_RIGHT_SCROLL_TOGGLE, SV_SNIPER_2        , SV_RIGHT_DPI_DEC    , KC_TRNS            , KC_NO ,
        /*L3*/ SV_LEFT_DPI_INC     , SV_LEFT_SCROLL_TOGGLE, SV_SNIPER_3         , SV_LEFT_DPI_DEC     , KC_TRNS            , KC_NO ,
        /*L4*/ SV_SCROLL_HOLD      , SV_SCROLL_TOGGLE     , SV_AXIS_SCROLL_LOCK , KC_TRNS             , KC_TRNS            , KC_NO ,

        /*     Down            Pad      Up       Nail     Knuckle  DoubleDown */
        /*RT*/ KC_TRNS       , KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS ,
        /*LT*/ SV_LOCK_MBO   , KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    ),

    [MBO] = LAYOUT(
        /*     Center                  North    East     South       West     Double */
        /*R1*/ SV_MBO_SFT            , SV_LEFT_DPI_DEC        , KC_TRNS, KC_BTN1      , KC_TRNS, KC_NO ,
        /*R2*/ SV_MBO_GUI            , SV_LEFT_DPI_INC        , KC_TRNS, KC_BTN2      , KC_TRNS, KC_NO ,
        /*R3*/ SV_MBO_ALT            , SV_LEFT_SCROLL_TOGGLE  , KC_TRNS, SV_SNIPER_3  , KC_TRNS, KC_NO ,
        /*R4*/ SV_MBO_CTL            , KC_TRNS                , KC_TRNS, SV_BOOST_2   , KC_TRNS, KC_NO ,
        /*L1*/ KC_TRNS               , SV_RIGHT_DPI_DEC       , KC_TRNS, KC_BTN1      , KC_TRNS, KC_NO ,
        /*L2*/ KC_TRNS               , SV_RIGHT_DPI_INC       , KC_TRNS, KC_BTN2      , KC_TRNS, KC_NO ,
        /*L3*/ KC_TRNS               , SV_RIGHT_SCROLL_TOGGLE , KC_TRNS, SV_SNIPER_3  , KC_TRNS, KC_NO ,
        /*L4*/ KC_TRNS               , KC_TRNS                , KC_TRNS, SV_BOOST_2   , KC_TRNS, KC_NO ,

        /*     Down            Pad      Up       Nail     Knuckle  DoubleDown */
        /*RT*/ KC_TRNS       , KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS ,
        /*LT*/ KC_TRNS       , KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    ),
};
#endif

void keyboard_post_init_user(void) {
  rgblight_layers = kvm_rgb_layers;
  rgblight_set_layer_state(0, true); /* machine 1 (green) active at boot */

#ifdef QMK_SETTINGS
  sync_runtime_qmk_settings();
#endif

#if __has_include("keymap_all.h")
  if (fresh_install) {
    sval_init_defaults();
  }
#endif
}
