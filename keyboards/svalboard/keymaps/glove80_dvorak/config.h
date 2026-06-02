/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#define STENO_COMBINEDMAP

#define VIAL_KEYBOARD_UID {0x1B, 0x18, 0x7D, 0xF2, 0x21, 0xF6, 0x29, 0x48}

// The RP2040 build has no dedicated USB VBUS sense pin, so QMK uses
// SPLIT_USB_DETECT and waits for active USB before deciding which half is
// master. The stock 2s USB window and 2.1s watchdog are tight enough that a
// slow host/KVM/hub or still-initializing slave can leave only one half alive.
#define SPLIT_USB_TIMEOUT 3000
#define SPLIT_WATCHDOG_TIMEOUT 10000

// Vial security combos, depending on which unit this is...
#ifdef INIT_EE_HANDS_RIGHT
// right thumb lock
#define VIAL_UNLOCK_COMBO_ROWS { 5, 5 }
#define VIAL_UNLOCK_COMBO_COLS { 0, 1 }
#elif INIT_EE_HANDS_LEFT
// left thumb lock
#define VIAL_UNLOCK_COMBO_ROWS { 0, 0 }
#define VIAL_UNLOCK_COMBO_COLS { 0, 1 }
#else
// both thumb locks
#define VIAL_UNLOCK_COMBO_ROWS { 0, 0, 5, 5 }
#define VIAL_UNLOCK_COMBO_COLS { 2, 5, 2, 5 }
#endif

// Shorten the unlock timeout (needs mod in `quantum/vial.c`; without
// it the override doesn't work)
#define VIAL_UNLOCK_COUNTER_MAX 12

// Slightly longer than the stock 200 ms to better match the user's Glove80
// home-row-mod timing on the Svalboard's very light center keys.
#ifdef TAPPING_TERM
#undef TAPPING_TERM
#endif
#define TAPPING_TERM 225

// Fallback idle timeout for thumb Down/DoubleDown disambiguation. A non-thumb
// key press now commits Down immediately; this only applies while Down is held
// alone.
#define THUMB_DOUBLEDOWN_GRACE_MS 100

// When the temporary scroll swap is active, scale each physical trackball's
// report so the pointer and scroller keep their usual sensitivities.
#define SVALBOARD_SCALE_DPI_WITH_SCROLL_SWAP

// When a mouse button key is pressed, briefly suppress tiny pointer motion so
// click keys do not become accidental drags while the trackball hand settles.
#define SVALBOARD_MOUSE_CLICK_GUARD_MS 50
#define SVALBOARD_MOUSE_CLICK_GUARD_THRESHOLD 30
