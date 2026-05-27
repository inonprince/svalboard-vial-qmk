/*
Copyright 2012-2018 Jun Wako, Jack Humbert, Yiancar

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
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "util.h"
#include "matrix.h"
#include "debounce.h"
#include "quantum.h"
#include "print.h"
#include "svalboard.h"
#ifdef SPLIT_KEYBOARD
#    include "split_common/transport.h"
#    include "split_common/split_util.h"
#endif

#define ROWS_PER_HAND 5

// matrix code
const pin_t col_pins[MATRIX_COLS] = MATRIX_COL_PINS;
const pin_t row_pins[ROWS_PER_HAND] = MATRIX_ROW_PINS;
//static const uint8_t col_pushed_states[MATRIX_COLS] = MATRIX_COL_PUSHED_STATES;
static const uint8_t col_pushed_states_fingers[MATRIX_COLS] = MATRIX_COL_PUSHED_STATES;
static const uint8_t col_pushed_states_thumbs[MATRIX_COLS] = MATRIX_COL_PUSHED_STATES_THUMBS;

static inline void setPinOutput_writeLow(pin_t pin) {
    ATOMIC_BLOCK_FORCEON {
        setPinOutput(pin);
        writePinLow(pin);
    }
}

static inline void setPinOutput_writeHigh(pin_t pin) {
    ATOMIC_BLOCK_FORCEON {
        setPinOutput(pin);
        writePinHigh(pin);
    }
}

static inline void setPinInputHigh_atomic(pin_t pin) {
    ATOMIC_BLOCK_FORCEON {
        setPinInputHigh(pin);
    }
}

static inline uint8_t readMatrixPin(pin_t pin) {
    if (pin != NO_PIN) {
        return (readPin(pin));
    } else {
        return 1;
    }
}


bool select_row(uint8_t row) {
    pin_t pin = row_pins[row];
    if (pin != NO_PIN) {
#ifdef PFET_ROWS
        setPinOutput_writeLow(pin);  //
        return true;
#else
        setPinOutput_writeHigh(pin);  //  this is opposite of most KB matrices
        return true;
#endif
    }
    return false;
}

void unselect_row(uint8_t row) {
    pin_t pin = row_pins[row];
#ifdef PFET_ROWS
            setPinOutput_writeHigh(pin);
#else
            setPinOutput_writeLow(pin);
#endif
}


static void unselect_rows(void) {
    for (uint8_t x = 0; x < ROWS_PER_HAND; x++) {
        unselect_row(x);
    }
}


/* matrix state(1:on, 0:off) */
extern matrix_row_t raw_matrix[MATRIX_ROWS]; // raw values
extern matrix_row_t matrix[MATRIX_ROWS];     // filtered debounced values
#ifdef SPLIT_KEYBOARD
extern uint8_t thisHand, thatHand;
#endif

extern uint16_t sval_prewait_us[];
extern uint16_t sval_postwait_us[];

#define THUMB_DOWN_COL 2
#ifndef THUMB_DOUBLEDOWN_GRACE_MS
#define THUMB_DOUBLEDOWN_GRACE_MS 50
#endif
/* Minimum time (ms) to hold the synthetic Down press during tap replay. */
#ifndef THUMB_TAP_REPLAY_MS
#define THUMB_TAP_REPLAY_MS 10
#endif

typedef enum {
    THUMB_CLUSTER_IDLE,
    THUMB_CLUSTER_PENDING,
    THUMB_CLUSTER_DOWN_COMMITTED,
    THUMB_CLUSTER_DD_COMMITTED,
    THUMB_CLUSTER_TAP_REPLAY,
} thumb_cluster_state_t;

typedef enum {
    THUMB_RAW_IDLE,
    THUMB_RAW_DOWN,
    THUMB_RAW_TAP_REPLAY,
} thumb_raw_state_t;

/*
 * Gen 2 boards can invert the idle polarity of the double-down switch. Sample
 * the idle level a few scans after boot, then normalize the column by XORing
 * future reads against that baseline.
 */
static int16_t scans_before_dd_detect = 3;  // Has to be one higher than the actual number of scans.
static uint8_t dd_detected = 0;

/*
 * The thumb Down and DoubleDown positions are a single physical key with two
 * switches. The debounced physical matrix is filtered after both halves have
 * been combined, so a cross-hand key press can commit Down immediately instead
 * of waiting for the DoubleDown fallback timer.
 */
typedef struct {
    thumb_cluster_state_t state;
    uint32_t              started_at;
} thumb_cluster_t;

static matrix_row_t   debounced_physical_matrix[MATRIX_ROWS];
static matrix_row_t   previous_physical_matrix[MATRIX_ROWS];
static matrix_row_t   delayed_press_matrix[MATRIX_ROWS];
static thumb_cluster_t thumb_clusters[] = {
    {THUMB_CLUSTER_IDLE, 0},
    {THUMB_CLUSTER_IDLE, 0},
};

static const char * const thumb_state_names[] __attribute__((unused)) = {
    "IDLE", "PEND", "DOWN", "DD", "RPLY"
};

static thumb_raw_state_t thumb_raw_state = THUMB_RAW_IDLE;
static uint32_t          thumb_raw_replay_started_at = 0;

static void apply_thumb_down_tap_replay(matrix_row_t *current_row_value) {
    const matrix_row_t down_mask           = (matrix_row_t)1 << THUMB_DOWN_COL;
    const matrix_row_t double_down_mask    = (matrix_row_t)1 << DOUBLEDOWN_COL;
    const bool         raw_down_pressed    = (*current_row_value & down_mask) != 0;
    const bool         double_down_pressed = (*current_row_value & double_down_mask) != 0;

    switch (thumb_raw_state) {
        case THUMB_RAW_IDLE:
            if (raw_down_pressed && !double_down_pressed) {
                thumb_raw_state = THUMB_RAW_DOWN;
            }
            break;

        case THUMB_RAW_DOWN:
            if (double_down_pressed) {
                thumb_raw_state = THUMB_RAW_IDLE;
            } else if (!raw_down_pressed) {
                thumb_raw_state             = THUMB_RAW_TAP_REPLAY;
                thumb_raw_replay_started_at = timer_read32();
                *current_row_value |= down_mask;
            }
            break;

        case THUMB_RAW_TAP_REPLAY:
            if (double_down_pressed) {
                *current_row_value &= ~down_mask;
                thumb_raw_state = THUMB_RAW_IDLE;
            } else if (timer_elapsed32(thumb_raw_replay_started_at) >= THUMB_TAP_REPLAY_MS) {
                *current_row_value &= ~down_mask;
                thumb_raw_state = raw_down_pressed ? THUMB_RAW_DOWN : THUMB_RAW_IDLE;
            } else {
                *current_row_value |= down_mask;
            }
            break;
    }
}

static bool collect_new_non_down_dd_presses(const matrix_row_t physical_matrix[], matrix_row_t thumb_cluster_mask, matrix_row_t new_presses[]) {
    bool found = false;

    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        matrix_row_t row_presses = physical_matrix[row] & ~previous_physical_matrix[row];
        if (row == 0 || row == ROWS_PER_HAND) {
            row_presses &= ~thumb_cluster_mask;
        }
        new_presses[row] = row_presses;
        found |= row_presses != 0;
    }

    return found;
}

static void suppress_presses(matrix_row_t output_matrix[], const matrix_row_t suppressed_presses[]) {
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        output_matrix[row] &= ~suppressed_presses[row];
        delayed_press_matrix[row] |= suppressed_presses[row];
    }
}

static bool apply_thumb_cluster_interlock(uint8_t cluster_index, uint8_t thumb_row, const matrix_row_t physical_matrix[], matrix_row_t output_matrix[]) {
    const matrix_row_t down_mask         = (matrix_row_t)1 << THUMB_DOWN_COL;
    const matrix_row_t double_down_mask  = (matrix_row_t)1 << DOUBLEDOWN_COL;
    const bool         raw_down_pressed  = (physical_matrix[thumb_row] & down_mask) != 0;
    const bool         double_down_pressed = (physical_matrix[thumb_row] & double_down_mask) != 0;
    thumb_cluster_t   *cluster           = &thumb_clusters[cluster_index];
    const thumb_cluster_state_t prev_state = cluster->state;
    matrix_row_t       new_presses[MATRIX_ROWS] = {0};

    if (double_down_pressed) {
        output_matrix[thumb_row] &= ~down_mask;
    }

    switch (cluster->state) {
        case THUMB_CLUSTER_IDLE:
            if (double_down_pressed) {
                cluster->state = THUMB_CLUSTER_DD_COMMITTED;
            } else if (raw_down_pressed) {
                cluster->state      = THUMB_CLUSTER_PENDING;
                cluster->started_at = timer_read32();
                output_matrix[thumb_row] &= ~down_mask;
            }
            break;

        case THUMB_CLUSTER_PENDING:
            if (double_down_pressed) {
                cluster->state = THUMB_CLUSTER_DD_COMMITTED;
            } else if (!raw_down_pressed) {
                /* Quick tap: Down was released within the grace window
                 * without DoubleDown firing.  Replay the suppressed press
                 * so QMK sees a normal tap. */
                cluster->state      = THUMB_CLUSTER_TAP_REPLAY;
                cluster->started_at = timer_read32();
                output_matrix[thumb_row] |= down_mask;
            } else if (collect_new_non_down_dd_presses(physical_matrix, down_mask | double_down_mask, new_presses)) {
                /* A real key is trying to use the Down hold.  Commit Down now,
                 * and hold the new press back for one scan so QMK processes
                 * the layer-tap before the key that depends on it. */
                cluster->state      = THUMB_CLUSTER_DOWN_COMMITTED;
                cluster->started_at = timer_read32();
                output_matrix[thumb_row] |= down_mask;
                suppress_presses(output_matrix, new_presses);
            } else if (timer_elapsed32(cluster->started_at) < THUMB_DOUBLEDOWN_GRACE_MS) {
                output_matrix[thumb_row] &= ~down_mask;
            } else {
                cluster->state      = THUMB_CLUSTER_DOWN_COMMITTED;
                cluster->started_at = timer_read32();
            }
            break;

        case THUMB_CLUSTER_DOWN_COMMITTED:
            if (double_down_pressed) {
                cluster->state = THUMB_CLUSTER_DD_COMMITTED;
            } else if (!raw_down_pressed) {
                cluster->state = THUMB_CLUSTER_IDLE;
            }
            break;

        case THUMB_CLUSTER_DD_COMMITTED:
            output_matrix[thumb_row] &= ~down_mask;
            if (!raw_down_pressed && !double_down_pressed) {
                cluster->state = THUMB_CLUSTER_IDLE;
            }
            break;

        case THUMB_CLUSTER_TAP_REPLAY:
            if (double_down_pressed) {
                output_matrix[thumb_row] &= ~down_mask;
                cluster->state = THUMB_CLUSTER_DD_COMMITTED;
            } else if (timer_elapsed32(cluster->started_at) >= THUMB_TAP_REPLAY_MS) {
                /* Replay complete — force OFF and return to idle. */
                output_matrix[thumb_row] &= ~down_mask;
                cluster->state = THUMB_CLUSTER_IDLE;
            } else {
                /* Hold the synthetic press long enough for QMK to see it. */
                output_matrix[thumb_row] |= down_mask;
            }
            break;
    }

    if (cluster->state != prev_state) {
        uprintf("TD%u: %s->%s raw=%d out=%d t=%lu\n",
                cluster_index,
                thumb_state_names[prev_state],
                thumb_state_names[cluster->state],
                raw_down_pressed,
                (output_matrix[thumb_row] & down_mask) ? 1 : 0,
                (unsigned long)timer_read32());
    }

    return cluster->state != prev_state;
}

static bool apply_thumb_double_down_interlock(matrix_row_t output_matrix[]) {
    matrix_row_t physical_matrix[MATRIX_ROWS];
    matrix_row_t delayed_matrix[MATRIX_ROWS];
    memcpy(physical_matrix, output_matrix, sizeof(physical_matrix));
    memcpy(delayed_matrix, delayed_press_matrix, sizeof(delayed_matrix));
    memset(delayed_press_matrix, 0, sizeof(delayed_press_matrix));

    bool changed = false;
    changed |= apply_thumb_cluster_interlock(0, 0, physical_matrix, output_matrix);
    changed |= apply_thumb_cluster_interlock(1, ROWS_PER_HAND, physical_matrix, output_matrix);

    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        output_matrix[row] |= delayed_matrix[row];
    }

    changed |= memcmp(previous_physical_matrix, physical_matrix, sizeof(physical_matrix)) != 0;
    memcpy(previous_physical_matrix, physical_matrix, sizeof(previous_physical_matrix));
    return changed;
}

void matrix_read_cols_on_row(matrix_row_t current_matrix[], uint8_t current_row) {
    // Start with a clear matrix row
    matrix_row_t current_row_value = 0;

    select_row(current_row);
    // if thumb row use col_pushed_states_thumbs

    wait_us(sval_prewait_us[global_saved_values.turbo_scan]);

    // For each col...
    for (uint8_t col_index = 0; col_index < MATRIX_COLS; col_index++) {
        uint8_t pin_state;
        if (current_row == 0) {
            pin_state = (readPin(col_pins[col_index]) == col_pushed_states_thumbs[col_index]) ? 1 : 0;  // read pin and match pushed_states define
	    if (col_index == 5) { // DD
		if (scans_before_dd_detect >= 0) {
                   scans_before_dd_detect--;
		}
		if (!scans_before_dd_detect && scans_before_dd_detect == 0) {
		    dd_detected = pin_state;
		    scans_before_dd_detect--;
		}
		pin_state ^= dd_detected;
		pin_state &= 1;
	    }
        } else {
               pin_state = (readPin(col_pins[col_index]) == col_pushed_states_fingers[col_index]) ? 1 : 0; // read pin and match pushed_states define
        }
	// Populate the matrix row with the state of the col pin
        current_row_value |= (pin_state << col_index);
    }

    if (current_row == 0) {
        apply_thumb_down_tap_replay(&current_row_value);
    }

    // Unselect row
    unselect_row(current_row);
    wait_us(sval_postwait_us[global_saved_values.turbo_scan]);

    // Update the matrix
    current_matrix[current_row] = current_row_value;
}

void matrix_init_custom(void) {
    print("matrix_init_custom\n");
    unselect_rows();
    for (uint8_t col = 0; col < MATRIX_COLS; col++) {
        pin_t pin = col_pins[col];
        if (pin != NO_PIN) {
            if (col == DOUBLEDOWN_COL){
                setPinInputHigh(pin);
            } else {
                setPinInput(pin);
            }
        }
    }
}
bool first_scan = true;
bool matrix_scan_custom(matrix_row_t current_matrix[]) {
    
    matrix_row_t curr_matrix[ROWS_PER_HAND] = {0};
    // Set row, read cols
    for (uint8_t current_row = 0; current_row < (ROWS_PER_HAND); current_row++) {
        matrix_read_cols_on_row(curr_matrix, current_row);
    }

    // The first scan comes out backwards for reasons we don't understand.
    // Skipping it, stops the lockup experienced with thumbs out on
    // board boot.
    if (first_scan) {
        memset(raw_matrix, 0, sizeof(raw_matrix));
        first_scan = false;
	return true;
    } else {
        bool changed = memcmp(raw_matrix, curr_matrix, sizeof(curr_matrix)) != 0;
        if (changed) memcpy(raw_matrix, curr_matrix, sizeof(curr_matrix));
	return changed;
    }
}

uint8_t matrix_scan(void) {
    bool changed = matrix_scan_custom(raw_matrix);

#ifdef SPLIT_KEYBOARD
    changed = debounce(raw_matrix, debounced_physical_matrix + thisHand, ROWS_PER_HAND, changed);

    if (is_keyboard_master()) {
        static bool  last_connected              = false;
        matrix_row_t slave_matrix[ROWS_PER_HAND] = {0};

        if (transport_master_if_connected(debounced_physical_matrix + thisHand, slave_matrix)) {
            bool slave_changed = memcmp(debounced_physical_matrix + thatHand, slave_matrix, sizeof(slave_matrix)) != 0;
            if (slave_changed) {
                memcpy(debounced_physical_matrix + thatHand, slave_matrix, sizeof(slave_matrix));
            }
            changed |= slave_changed;
            last_connected = true;
        } else if (last_connected) {
            memset(debounced_physical_matrix + thatHand, 0, sizeof(slave_matrix));
            changed = true;
            last_connected = false;
        }

        memcpy(matrix, debounced_physical_matrix, sizeof(debounced_physical_matrix));
        changed |= apply_thumb_double_down_interlock(matrix);
        matrix_scan_kb();
    } else {
        transport_slave(debounced_physical_matrix + thatHand, debounced_physical_matrix + thisHand);
        matrix_slave_scan_kb();
    }
#else
    changed = debounce(raw_matrix, debounced_physical_matrix, ROWS_PER_HAND, changed);
    memcpy(matrix, debounced_physical_matrix, sizeof(debounced_physical_matrix));
    changed |= apply_thumb_double_down_interlock(matrix);
    matrix_scan_kb();
#endif

    return changed;
}
