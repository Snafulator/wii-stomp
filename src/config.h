/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef WII_STOMP_CONFIG_H
#define WII_STOMP_CONFIG_H

#include <stdio.h>

/* Tuning. Built-in defaults, overridden in turn by settings.json next to the
 * program, ~/.config/wii-stomp/settings.json, -s file, and the profile's
 * "settings" block. calibrate.py writes ~/.config/wii-stomp/settings.json. */
struct settings {
	int x_mult;      /* left/right sensitivity */
	int y_mult;      /* forward/back sensitivity (negative = inverted) */
	int x_deadzone;  /* left/right output ignored below this */
	/* forward/back goes full tilt once lean passes ON, drops back to centre below OFF.
	 * Separate values because pushing with toes is weaker than leaning on heels. */
	int fwd_on, fwd_off, back_on, back_off;
	int settle_ms;   /* ms after feet go down to ignore while they settle */
	int calib_ms;    /* ms after settling to measure resting balance */
	int min_weight;  /* total below this = feet off, output centred (empty board reads ~90) */
	/* bash: one side of the board (front + back sensor) jumps up fast */
	int bash_window_ms;   /* the jump must happen within this long */
	int bash_rise;        /* that side must rise this much above its low in the window (seated, whole side: bashes ~500-1370) */
	int bash_margin;      /* ...and this much more than the other side, so shifting weight around doesn't count */
	int bash_jolt;        /* the other side must rise at least this much too: a bash jolts the whole board (seen 66+), leaning takes weight off it (0) */
	int bash_hold_ms;     /* how long the button stays pressed */
	int bash_cooldown_ms; /* ignore that side for this long after a bash */
};
extern struct settings cfg;

/* what an input does: press a key/button (for as long as the input is held),
 * or toggle lean movement */
enum { ACT_NONE, ACT_PRESS, ACT_TOGGLE_LEAN };
struct action { int type, code, fd; };

/* the active profile */
enum { DEV_XBOX, DEV_KBM };
struct profile {
	char name[64];
	int device; /* DEV_XBOX: virtual Xbox 360 pad (lean drives its stick); DEV_KBM: keyboard/mouse */
	int lean;   /* lean movement on at start */
	struct action bash_l, bash_r, button;
};
extern struct profile prof;

/* names usable in profiles: pad buttons, mouse buttons, and Linux key names without KEY_ */
struct keyname { const char *name; int code; };
extern const struct keyname keynames[];
extern const int nkeynames;

int is_pad_button(int code);

/* path: -s file, or NULL for just the standard locations */
int config_load_settings(const char *path);
/* a profile name (profiles/<name>.json in ~/.config/wii-stomp or next to the program) or a .json path */
int config_load_profile(const char *arg);
/* the current settings, as a settings.json */
void config_print_settings(FILE *f);

#endif
