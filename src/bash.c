/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdio.h>

#include "bash.h"
#include "board.h"
#include "config.h"
#include "output.h"

#define HIST 64 /* ~0.6s of samples at the board's ~100Hz */
static struct { long t; int l, r; } hist[HIST];
static int hist_i;

struct heel { struct action *act; const char *name; int down; long at; };
static struct heel heel_l = { .act = &prof.bash_l, .name = "left" }, heel_r = { .act = &prof.bash_r, .name = "right" };

static void checkHeel(struct heel *h, int rise, int other_rise, long now)
{
	if (h->down){
		if (now - h->at >= cfg.bash_hold_ms){
			h->down = 0;
			output_action(h->act, 0);
		}
		return;
	}
	if (calibrating || now - h->at < cfg.bash_cooldown_ms)
		return;
	if (rise >= cfg.bash_rise && rise - other_rise >= cfg.bash_margin && other_rise >= cfg.bash_jolt){
		h->at = now;
		fprintf(stderr, "%s bash (rise %d, other %d)\n", h->name, rise, other_rise);
		h->down = h->act->type == ACT_PRESS;
		output_action(h->act, 1);
	}
}

void bash_update(void)
{
	long now = now_ms();
	int l = weight.fl + weight.bl, r = weight.fr + weight.br; /* each side's total */
	hist[hist_i].t = now;
	hist[hist_i].l = l;
	hist[hist_i].r = r;
	hist_i = (hist_i + 1) % HIST;

	/* lowest each side read during the window; a bash is a sharp rise from it */
	int low_l = l, low_r = r;
	for (int i = 0; i < HIST; i++){
		if (now - hist[i].t > cfg.bash_window_ms) continue;
		if (hist[i].l < low_l) low_l = hist[i].l;
		if (hist[i].r < low_r) low_r = hist[i].r;
	}
	int rise_l = l - low_l, rise_r = r - low_r;

	checkHeel(&heel_l, rise_l, rise_r, now);
	checkHeel(&heel_r, rise_r, rise_l, now);
}
