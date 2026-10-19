/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef WII_STOMP_OUTPUT_H
#define WII_STOMP_OUTPUT_H

#include "config.h"

/* Virtual devices. Gamepad buttons and the lean stick go out on a virtual
 * Xbox 360 pad; keys and mouse buttons on a separate keyboard/mouse device
 * (its keys are set by the profile, so no Steam Input remapping is needed,
 * e.g. under gamescope). */

extern int lean_on; /* lean movement drives the stick; toggle_lean flips it */

/* creates the profile's device */
int output_open(void);
/* value 1 = input pressed, 0 = released: press/release a key, or toggle lean on press */
void output_action(struct action *a, int value);
/* moves the pad's left stick (centred while lean is off) */
int output_stick(int x, int y);
void output_close(void);

#endif
