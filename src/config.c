/* SPDX-License-Identifier: GPL-3.0-only */
#include <linux/input.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "config.h"
#include "json.h"

struct settings cfg = {
	.x_mult = 9, .y_mult = 3, .x_deadzone = 100,
	.fwd_on = 600, .fwd_off = 400, .back_on = 1500, .back_off = 1100,
	.settle_ms = 700, .calib_ms = 1000, .min_weight = 400,
	.bash_window_ms = 150, .bash_rise = 400, .bash_margin = 300, .bash_jolt = 40,
	.bash_hold_ms = 100, .bash_cooldown_ms = 300,
};
#define S(n) { #n, &cfg.n }
static const struct { const char *name; int *val; } settings[] = {
	S(x_mult), S(y_mult), S(x_deadzone), S(fwd_on), S(fwd_off), S(back_on), S(back_off),
	S(settle_ms), S(calib_ms), S(min_weight), S(bash_window_ms), S(bash_rise),
	S(bash_margin), S(bash_jolt), S(bash_hold_ms), S(bash_cooldown_ms),
};
#define NSETTINGS (int)(sizeof(settings) / sizeof(settings[0]))

/* used when no -p is given */
struct profile prof = {
	.name = "xbox (built in)", .device = DEV_XBOX, .lean = 1,
	.bash_l = { ACT_PRESS, BTN_TL }, .bash_r = { ACT_PRESS, BTN_TR },
	.button = { ACT_TOGGLE_LEAN },
};

#define K(n) { #n, KEY_##n }
const struct keyname keynames[] = {
	{ "PAD_A", BTN_A }, { "PAD_B", BTN_B }, { "PAD_X", BTN_X }, { "PAD_Y", BTN_Y },
	{ "LB", BTN_TL }, { "RB", BTN_TR }, { "BACK", BTN_SELECT }, { "START", BTN_START },
	{ "GUIDE", BTN_MODE }, { "LS", BTN_THUMBL }, { "RS", BTN_THUMBR },
	{ "MOUSE1", BTN_LEFT }, { "MOUSE2", BTN_RIGHT }, { "MOUSE3", BTN_MIDDLE },
	{ "MOUSE4", BTN_SIDE }, { "MOUSE5", BTN_EXTRA },
	{ "SHIFT", KEY_LEFTSHIFT }, { "CTRL", KEY_LEFTCTRL }, { "ALT", KEY_LEFTALT },
	K(A), K(B), K(C), K(D), K(E), K(F), K(G), K(H), K(I), K(J), K(K), K(L), K(M),
	K(N), K(O), K(P), K(Q), K(R), K(S), K(T), K(U), K(V), K(W), K(X), K(Y), K(Z),
	K(0), K(1), K(2), K(3), K(4), K(5), K(6), K(7), K(8), K(9),
	K(F1), K(F2), K(F3), K(F4), K(F5), K(F6), K(F7), K(F8), K(F9), K(F10), K(F11), K(F12),
	K(SPACE), K(ENTER), K(ESC), K(TAB), K(BACKSPACE), K(CAPSLOCK),
	K(LEFTSHIFT), K(RIGHTSHIFT), K(LEFTCTRL), K(RIGHTCTRL), K(LEFTALT), K(RIGHTALT),
	K(UP), K(DOWN), K(LEFT), K(RIGHT), K(INSERT), K(DELETE), K(HOME), K(END),
	K(PAGEUP), K(PAGEDOWN), K(MINUS), K(EQUAL), K(LEFTBRACE), K(RIGHTBRACE),
	K(SEMICOLON), K(APOSTROPHE), K(GRAVE), K(BACKSLASH), K(COMMA), K(DOT), K(SLASH),
};
const int nkeynames = sizeof(keynames) / sizeof(keynames[0]);

#define MAXKV 32
static struct json_kv kv[MAXKV];

int is_pad_button(int code)
{
	return code >= BTN_GAMEPAD && code <= BTN_THUMBR;
}

/* Where settings.json and profiles/ are looked for, most important first:
 * ~/.config/wii-stomp, next to the program, and ../share/wii-stomp from it (installed). */
#define NDIRS 3
static char dirs[NDIRS][4096];

static void findDirs(void)
{
	const char *xdg = getenv("XDG_CONFIG_HOME"), *home = getenv("HOME");
	if (xdg && *xdg)
		snprintf(dirs[0], sizeof(dirs[0]), "%.4000s/wii-stomp", xdg);
	else if (home)
		snprintf(dirs[0], sizeof(dirs[0]), "%.4000s/.config/wii-stomp", home);
	char exe[4000];
	ssize_t len = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
	if (len > 0){
		exe[len] = 0;
		*strrchr(exe, '/') = 0;
		snprintf(dirs[1], sizeof(dirs[1]), "%s", exe);
		snprintf(dirs[2], sizeof(dirs[2]), "%s/../share/wii-stomp", exe);
	}
}

/* "none", "toggle_lean", or a key/button name from keynames */
static int parseAction(const char *s, struct action *a)
{
	if (!strcasecmp(s, "none")){ a->type = ACT_NONE; return 0; }
	if (!strcasecmp(s, "toggle_lean")){ a->type = ACT_TOGGLE_LEAN; return 0; }
	for (int i = 0; i < nkeynames; i++)
		if (!strcasecmp(s, keynames[i].name)){
			a->type = ACT_PRESS;
			a->code = keynames[i].code;
			return 0;
		}
	return -1;
}

static int setSetting(const char *path, const char *k, const char *v)
{
	for (int i = 0; i < NSETTINGS; i++)
		if (!strcmp(k, settings[i].name)){
			char *end;
			long n = strtol(v, &end, 10);
			if (*end || end == v){
				fprintf(stderr, "%s: %s: '%s' isn't a whole number\n", path, k, v);
				return -1;
			}
			*settings[i].val = n;
			return 0;
		}
	fprintf(stderr, "%s: unknown setting '%s'\n", path, k);
	return -1;
}

static int applySettingsFile(FILE *f, const char *path)
{
	int n = json_read(f, path, kv, MAXKV);
	if (n < 0)
		return -1;
	for (int i = 0; i < n; i++)
		if (setSetting(path, kv[i].key, kv[i].val))
			return -1;
	fprintf(stderr, "settings from %s\n", path);
	return 0;
}

int config_load_settings(const char *arg)
{
	findDirs();
	/* least important first, so later files win */
	for (int i = NDIRS - 1; i >= 0; i--){
		char path[4200];
		if (!*dirs[i])
			continue;
		snprintf(path, sizeof(path), "%.4000s/settings.json", dirs[i]);
		FILE *f = fopen(path, "r");
		if (f && applySettingsFile(f, path))
			return -1;
	}
	if (arg){
		FILE *f = fopen(arg, "r");
		if (!f){
			perror(arg);
			return -1;
		}
		if (applySettingsFile(f, arg))
			return -1;
	}
	return 0;
}

/* every binding has to exist on the profile's device */
static int checkProfile(const char *path)
{
	const char *names[] = { "bash.left", "bash.right", "button" };
	struct action *acts[] = { &prof.bash_l, &prof.bash_r, &prof.button };
	if (prof.device == DEV_KBM && prof.lean){
		fprintf(stderr, "%s: lean needs \"device\": \"xbox\" (a kbm profile has no stick)\n", path);
		return -1;
	}
	for (int i = 0; i < 3; i++){
		int pad = acts[i]->type == ACT_PRESS && is_pad_button(acts[i]->code);
		int key = acts[i]->type == ACT_PRESS && !pad;
		if (prof.device == DEV_KBM && (pad || acts[i]->type == ACT_TOGGLE_LEAN)){
			fprintf(stderr, "%s: %s: a kbm profile can only use keys and mouse buttons\n", path, names[i]);
			return -1;
		}
		if (prof.device == DEV_XBOX && key){
			fprintf(stderr, "%s: %s: an xbox profile can only use pad buttons and toggle_lean\n", path, names[i]);
			return -1;
		}
	}
	return 0;
}

int config_load_profile(const char *arg)
{
	char path[4200];
	FILE *f = NULL;
	if (strchr(arg, '/') || strstr(arg, ".json")){
		snprintf(path, sizeof(path), "%s", arg);
		f = fopen(path, "r");
	} else {
		findDirs();
		for (int i = 0; i < NDIRS && !f; i++){
			if (!*dirs[i])
				continue;
			snprintf(path, sizeof(path), "%.4000s/profiles/%.100s.json", dirs[i], arg);
			f = fopen(path, "r");
		}
	}
	if (!f){
		fprintf(stderr, "Can't find profile '%s' (looked in profiles/ in ~/.config/wii-stomp and next to the program)\n", arg);
		return -1;
	}
	int n = json_read(f, path, kv, MAXKV);
	if (n < 0)
		return -1;

	/* anything the file leaves out is unbound; lean defaults to on for xbox */
	memset(&prof, 0, sizeof(prof));
	prof.lean = -1;
	snprintf(prof.name, sizeof(prof.name), "%s", arg);
	for (int i = 0; i < n; i++){
		const char *k = kv[i].key, *v = kv[i].val;
		int bad = 0;
		if (!strcmp(k, "name"))
			snprintf(prof.name, sizeof(prof.name), "%s", v);
		else if (!strcmp(k, "description"))
			; /* for people reading the file */
		else if (!strcmp(k, "device")){
			if (!strcmp(v, "xbox")) prof.device = DEV_XBOX;
			else if (!strcmp(v, "kbm")) prof.device = DEV_KBM;
			else bad = 1;
		}
		else if (!strcmp(k, "lean")){
			if (!strcmp(v, "true")) prof.lean = 1;
			else if (!strcmp(v, "false")) prof.lean = 0;
			else bad = 1;
		}
		else if (!strcmp(k, "bash.left"))  bad = parseAction(v, &prof.bash_l);
		else if (!strcmp(k, "bash.right")) bad = parseAction(v, &prof.bash_r);
		else if (!strcmp(k, "button"))     bad = parseAction(v, &prof.button);
		else if (!strncmp(k, "settings.", 9)){
			if (setSetting(path, k + 9, v))
				return -1;
		}
		else {
			fprintf(stderr, "%s: unknown setting '%s'\n", path, k);
			return -1;
		}
		if (bad){
			fprintf(stderr, "%s: %s: can't use '%s'\n", path, k, v);
			return -1;
		}
	}
	if (prof.lean < 0)
		prof.lean = prof.device == DEV_XBOX;
	return checkProfile(path);
}

void config_print_settings(FILE *f)
{
	fprintf(f, "{\n");
	for (int i = 0; i < NSETTINGS; i++)
		fprintf(f, "\t\"%s\": %d%s\n", settings[i].name, *settings[i].val, i < NSETTINGS - 1 ? "," : "");
	fprintf(f, "}\n");
}
