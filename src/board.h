/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef WII_STOMP_BOARD_H
#define WII_STOMP_BOARD_H

#include "common.h"

/* Balance board state: sensor readings, feet on/off and resting-balance
 * calibration, and the stick position leaning gives. */
extern struct weights weight;
extern int calibrating; /* feet off, or still settling/measuring */

/* finds the board's /dev/input/event*; 0 if found */
int board_find(char *path, int n);
/* feed one EV_ABS event from the board */
void board_update(int code, int value);
/* stick position from leaning, +-AXIS_MAX; centred while calibrating */
void board_stick(int *x, int *y);

#endif
