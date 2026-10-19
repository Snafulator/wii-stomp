/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef WII_STOMP_BASH_H
#define WII_STOMP_BASH_H

/* Bash detection: one side of the board (front + back sensor) jumps up fast
 * -> that side's profile action. Call once per board report, when all four
 * sensors are current. */
void bash_update(void);

#endif
