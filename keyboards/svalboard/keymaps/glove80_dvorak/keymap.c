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

layer_state_t default_layer_state_set_user(layer_state_t state) {
  sval_set_active_layer(0, false);
  return state;
}

layer_state_t layer_state_set_user(layer_state_t state) {
  sval_set_active_layer(get_highest_layer(state), false);
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

enum custom_keycodes {
    SV_APP_SWITCH = QK_KB_20,
};

static bool app_switch_active = false;
static bool app_switch_added_gui = false;
static uint8_t app_switch_layer_tap_depth = 0;

static bool is_app_switch_layer_tap(uint16_t keycode) {
  return IS_QK_LAYER_TAP(keycode);
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
  if (app_switch_active && app_switch_added_gui && keycode != SV_APP_SWITCH &&
      record->event.pressed) {
    register_weak_mods(MOD_BIT(KC_LGUI));
  }

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
 * regardless of existing EEPROM state. Tapping term is adjusted to match the
 * user's Glove80-style home-row mod timing, and chordal hold stays enabled to
 * preserve the same-hand HRM suppression the Glove80 keymap used.
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
  set_qmk_setting_u8(QSID_PERMISSIVE_HOLD, 0);
  set_qmk_setting_u8(QSID_HOLD_ON_OTHER_KEY_PRESS, 0);
  set_qmk_setting_u8(QSID_RETRO_TAPPING, 0);
  set_qmk_setting_u16(QSID_QUICK_TAP_TERM, TAPPING_TERM);
  set_qmk_setting_u8(QSID_CHORDAL_HOLD, 1);
  set_qmk_setting_u16(QSID_FLOW_TAP_TERM, 0);
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
        /*R4*/ HM_S            , KC_L           , KC_MINS         , KC_Z            , KC_SLSH       , KC_NO ,
        /*L1*/ HM_U            , KC_P           , KC_I            , KC_K            , KC_Y          , KC_NO ,
        /*L2*/ HM_E            , KC_DOT         , S(KC_TAB)       , KC_J            , KC_GRV        , KC_NO ,
        /*L3*/ HM_O            , KC_COMM        , KC_X            , KC_Q            , KC_ESC        , KC_NO ,
        /*L4*/ HM_A            , KC_QUOT        , KC_BSLS         , KC_SCLN         , KC_DEL        , KC_NO ,

        /*     Down               Pad                 Up              Nail               Knuckle          DoubleDown */
        /*RT*/ LT(FUNC, KC_ENTER), LT(NAV, KC_SPACE), SV_APP_SWITCH , LT(NUM, KC_DEL)  , KC_LALT         , KC_LSFT ,
        /*LT*/ LT(SYS, KC_ESC)   , LT(SYM, KC_BSPC), KC_LGUI       , LT(MBO, KC_TAB)  , KC_LCTL         , SV_CAPS_WORD
    ),

    [NAV] = LAYOUT(
        /*     Center            North             East               South              West              Double */
        /*R1*/ KC_LSFT         , MAC_COPY        , MAC_PASTE        , SEL_WORD_RIGHT    , WORD_RIGHT      , KC_NO ,
        /*R2*/ KC_LGUI         , MAC_CUT         , MAC_FIND         , SEL_LINE_END      , LINE_END        , KC_NO ,
        /*R3*/ KC_LALT         , MAC_UNDO        , MAC_REDO         , SEL_LINE_START    , LINE_START      , KC_NO ,
        /*R4*/ KC_LCTL         , MAC_SAVE        , MAC_FIND_NEXT    , SEL_WORD_LEFT     , WORD_LEFT       , KC_NO ,
        /*L1*/ KC_RIGHT        , KC_END          , KC_TAB           , WORD_RIGHT        , KC_BSPC         , KC_NO ,
        /*L2*/ KC_UP           , KC_PGUP         , MAC_REDO         , LINE_END          , KC_DEL          , KC_NO ,
        /*L3*/ KC_DOWN         , KC_PGDN         , MAC_UNDO         , LINE_START        , KC_INS          , KC_NO ,
        /*L4*/ KC_LEFT         , KC_HOME         , S(KC_TAB)        , WORD_LEFT         , KC_ESC          , KC_NO ,

        /*     Down            Pad      Up       Nail     Knuckle  DoubleDown */
        /*RT*/ KC_TRNS       , KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS ,
        /*LT*/ KC_TRNS       , KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    ),

    [NUM] = LAYOUT(
        /*     Center            North             East              South             West              Double */
        /*R1*/ KC_LSFT         , S(KC_TAB)       , KC_TAB          , MAC_UNDO         , MAC_FIND        , KC_NO ,
        /*R2*/ KC_LGUI         , KC_DEL          , KC_BSPC         , MAC_REDO         , SEL_WORD_RIGHT  , KC_NO ,
        /*R3*/ KC_LALT         , KC_INS          , KC_SPACE        , MAC_FIND_PREV    , SEL_LINE_END    , KC_NO ,
        /*R4*/ KC_LCTL         , KC_ESC          , KC_ENTER        , MAC_FIND_NEXT    , MAC_SELECT_ALL  , KC_NO ,
        /*L1*/ KC_6            , KC_9            , HASH            , KC_3             , KC_SLSH         , KC_NO ,
        /*L2*/ KC_5            , KC_8            , AT              , KC_2             , KC_MINS         , KC_NO ,
        /*L3*/ KC_4            , KC_7            , EXLM            , KC_1             , PLUS            , KC_NO ,
        /*L4*/ KC_EQL          , ASTR            , RPRN            , UNDS             , LPRN            , KC_NO ,

        /*     Down            Pad      Up       Nail     Knuckle  DoubleDown */
        /*RT*/ KC_TRNS       , KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS ,
        /*LT*/ KC_0          , KC_COMM, KC_TRNS, KC_DOT , KC_TRNS, KC_TRNS
    ),

    [SYM] = LAYOUT(
        /*     Center            North             East              South             West              Double */
        /*R1*/ LPRN            , RPRN            , ASTR            , KC_SLSH          , EXLM            , KC_NO ,
        /*R2*/ KC_LBRC         , KC_RBRC         , AMPR            , AT               , HASH            , KC_NO ,
        /*R3*/ LCBR            , RCBR            , DLR             , PERC             , CIRC            , KC_NO ,
        /*R4*/ LTGT            , GTGT            , KC_EQL          , PLUS             , QUES            , KC_NO ,
        /*L1*/ KC_TAB          , S(KC_TAB)       , KC_DEL          , KC_BSPC          , KC_ESC          , KC_NO ,
        /*L2*/ KC_ENTER        , KC_INS          , DQUO            , KC_QUOT          , KC_GRV          , KC_NO ,
        /*L3*/ KC_SPACE        , COLN            , KC_DOT          , KC_SCLN          , KC_COMM         , KC_NO ,
        /*L4*/ KC_BSLS         , TILD            , PIPE            , KC_MINS          , UNDS            , KC_NO ,

        /*     Down            Pad      Up       Nail     Knuckle  DoubleDown */
        /*RT*/ KC_TRNS       , KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS ,
        /*LT*/ KC_TRNS       , KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
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
        /*RT*/ KC_TRNS       , KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS ,
        /*LT*/ KC_TRNS       , KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
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

        /*     Down            Pad      Up       Nail     Knuckle  DoubleDown */
        /*RT*/ KC_TRNS       , KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS ,
        /*LT*/ KC_TRNS       , KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    ),

    [TYPING] = LAYOUT(
        /*     Center            North    East     South    West     Double */
        /*R1*/ KC_H            , KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_NO ,
        /*R2*/ KC_T            , KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_NO ,
        /*R3*/ KC_N            , KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_NO ,
        /*R4*/ KC_S            , KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_NO ,
        /*L1*/ KC_U            , KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_NO ,
        /*L2*/ KC_E            , KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_NO ,
        /*L3*/ KC_O            , KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_NO ,
        /*L4*/ KC_A            , KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_NO ,

        /*     Down            Pad      Up             Nail     Knuckle  DoubleDown */
        /*RT*/ KC_TRNS       , KC_TRNS, SV_APP_SWITCH, KC_TRNS, KC_TRNS, KC_TRNS ,
        /*LT*/ KC_TRNS       , KC_TRNS, KC_TRNS       , KC_TRNS, KC_TRNS, KC_TRNS
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
        /*LT*/ KC_TRNS       , KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    ),

    [MBO] = LAYOUT(
        /*     Center                  North    East     South       West     Double */
        /*R1*/ KC_BTN1               , KC_TRNS, KC_TRNS, KC_TRNS   , KC_TRNS, KC_NO ,
        /*R2*/ KC_BTN3               , KC_TRNS, KC_TRNS, KC_TRNS   , KC_TRNS, KC_NO ,
        /*R3*/ KC_BTN2               , KC_TRNS, KC_TRNS, KC_TRNS   , KC_TRNS, KC_NO ,
        /*R4*/ SV_RECALIBRATE_POINTER, KC_TRNS, KC_TRNS, KC_TRNS   , KC_TRNS, KC_NO ,
        /*L1*/ KC_BTN1               , KC_TRNS, KC_TRNS, KC_TRNS   , KC_TRNS, KC_NO ,
        /*L2*/ KC_BTN3               , KC_TRNS, KC_TRNS, KC_TRNS   , KC_TRNS, KC_NO ,
        /*L3*/ KC_BTN2               , KC_TRNS, KC_TRNS, KC_TRNS   , KC_TRNS, KC_NO ,
        /*L4*/ SV_RECALIBRATE_POINTER, KC_TRNS, KC_TRNS, SV_SNIPER_3, KC_TRNS, KC_NO ,

        /*     Down            Pad      Up       Nail     Knuckle  DoubleDown */
        /*RT*/ KC_TRNS       , KC_BTN1, KC_TRNS, KC_BTN2, KC_TRNS, KC_TRNS ,
        /*LT*/ KC_TRNS       , KC_BTN1, KC_TRNS, KC_BTN2, KC_TRNS, KC_TRNS
    ),
};
#endif

void keyboard_post_init_user(void) {
  // Enable debug flags here if you need to inspect matrix or pointer behavior.

#ifdef QMK_SETTINGS
  sync_runtime_qmk_settings();
#endif

#if __has_include("keymap_all.h")
  if (fresh_install) {
    sval_init_defaults();
  }
#endif
}
