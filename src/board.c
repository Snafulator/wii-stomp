/* SPDX-License-Identifier: GPL-3.0-only */
#include <linux/input.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "board.h"
#include "config.h"

struct weights weight;
int calibrating = 1;

static int x, y;
static int x_offset, y_offset; /* resting raw x/y, subtracted after calibration */
static long calib_sum, calib_sum_y, calib_n, calib_k; /* n = samples since feet down, k = samples averaged */
static long start;

int board_find(char *path, int n)
{
	FILE *f = fopen("/proc/bus/input/devices", "r");
	if (!f)
		return -1;
	char line[512];
	int board = 0, found = -1;
	while (fgets(line, sizeof(line), f)){
		if (!strncmp(line, "N: ", 3))
			board = strstr(line, "Balance Board") != NULL;
		const char *ev;
		if (board && !strncmp(line, "H: ", 3) && (ev = strstr(line, "event"))){
			int num;
			if (sscanf(ev, "event%d", &num) == 1){
				snprintf(path, n, "/dev/input/event%d", num);
				found = 0;
			}
		}
	}
	fclose(f);
	return found;
}

static int clamp(int v)
{
	if (v > AXIS_MAX) return AXIS_MAX;
	if (v < -AXIS_MAX) return -AXIS_MAX;
	return v;
}

/* hid-wiimote reports the board's four corner sensors as hat axes */
static int *sensor(int code)
{
	static const struct { int code; size_t at; } corners[] = {
		{ ABS_HAT1X, offsetof(struct weights, fl) }, { ABS_HAT0X, offsetof(struct weights, fr) },
		{ ABS_HAT1Y, offsetof(struct weights, bl) }, { ABS_HAT0Y, offsetof(struct weights, br) },
	};
	for (unsigned i = 0; i < sizeof(corners) / sizeof(corners[0]); i++)
		if (corners[i].code == code)
			return (int *)((char *)&weight + corners[i].at);
	return NULL;
}

static int left(void)  { return weight.fl + weight.bl; }
static int right(void) { return weight.fr + weight.br; }
static int front(void) { return weight.fl + weight.fr; }
static int back(void)  { return weight.bl + weight.br; }

void board_update(int code, int value)
{
	int *s = sensor(code);
	if (!s)
		return;
	*s = value;
	int raw_x = right() - left();  /* + = leaning right */
	int raw_y = back() - front();  /* + = leaning back */

	if (left() + right() < cfg.min_weight){
		/* feet off: stay centred and recalibrate next time they go down */
		if (!calibrating)
			fprintf(stderr, "feet off\n");
		calibrating = 1;
		calib_n = 0;
		x = y = 0;
		return;
	}

	if (calibrating){
		if (calib_n == 0){
			calib_sum = calib_sum_y = calib_k = 0;
			start = now_ms();
			fprintf(stderr, "feet down, calibrating, keep still...\n");
		}
		calib_n++;
		if (now_ms() - start < cfg.settle_ms){
			x = y = 0;
			return;
		}
		calib_sum += raw_x;
		calib_sum_y += raw_y;
		calib_k++;
		if (now_ms() - start >= cfg.settle_ms + cfg.calib_ms){
			x_offset = calib_sum / calib_k;
			y_offset = calib_sum_y / calib_k;
			calibrating = 0;
			fprintf(stderr, "calibrated: centre x = %d, y = %d\n", x_offset, y_offset);
		}
		x = y = 0;
	} else {
		x=(raw_x-x_offset)*cfg.x_mult;
		if (x > -cfg.x_deadzone && x < cfg.x_deadzone) x = 0;
		/* forward/back is on/off: full tilt or centred */
		int lean=(raw_y-y_offset)*cfg.y_mult;
		int back_t=(y > 0) ? cfg.back_off : cfg.back_on; /* positive = backwards */
		int fwd_t=(y < 0) ? cfg.fwd_off : cfg.fwd_on;
		if (lean >= back_t) y = AXIS_MAX;
		else if (lean <= -fwd_t) y = -AXIS_MAX;
		else y = 0;
	}
	x=clamp(x);
	y=clamp(y);
}

void board_stick(int *sx, int *sy)
{
	*sx = x;
	*sy = y;
}
