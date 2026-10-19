/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef WII_STOMP_COMMON_H
#define WII_STOMP_COMMON_H

#include <time.h>

#define AXIS_MAX 8192 /* stick range, +-; must match the pad's ABS_X/ABS_Y setup in output.c */

/* the four load sensors, as last reported by the board */
struct weights { int fl, fr, bl, br; };

static inline long now_ms(void)
{
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return now.tv_sec * 1000 + now.tv_nsec / 1000000;
}

#endif
