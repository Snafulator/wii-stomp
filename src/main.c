/*
 * wii-stomp: turns a Wii Balance Board into a virtual Xbox 360 pad and/or
 * keyboard/mouse. Lean to move the left stick, bash a foot down to press a
 * button, and bind the board's power button. See README.md.
 *
 * Forked from https://github.com/g-rden/wbb-uinput, which is based on
 * https://github.com/keldu/WiiWeight (GPL-3.0) and
 * https://github.com/GrantEdwards/uinput-joystick-demo
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3. It comes with
 * NO WARRANTY. See the LICENSE file for details.
 */

#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "bash.h"
#include "board.h"
#include "config.h"
#include "output.h"

static void usage(const char *prog)
{
	fprintf(stderr,
		"Usage: %s [-p profile] [-s settings.json] [/dev/input/eventN]\n"
		"  -p  profile name (from profiles/) or path to a profile .json\n"
		"  -s  extra settings file, on top of ~/.config/wii-stomp/settings.json\n"
		"  -r  print raw sensor readings instead of creating devices (for calibrate.py)\n"
		"  -d  print the built-in default settings as JSON and exit\n"
		"The board is found automatically if no device is given.\n", prog);
}

/* -r: no devices, just print "ms fl fr bl br button" per board report */
static int raw(int fdr)
{
	struct input_event ev;
	int button = 0;
	while (read(fdr, &ev, sizeof(ev)) == sizeof(ev)){
		if (ev.type == EV_ABS)
			board_update(ev.code, ev.value);
		if (ev.type == EV_KEY && ev.code == BTN_A)
			button = ev.value != 0;
		if (ev.type == EV_SYN && ev.code == SYN_REPORT){
			printf("%ld %d %d %d %d %d\n", now_ms(), weight.fl, weight.fr, weight.bl, weight.br, button);
			fflush(stdout);
		}
	}
	return 0;
}

int main(int argc, char** argv)
{
	int opt, raw_mode = 0;
	const char *profile_arg = NULL, *settings_arg = NULL;
	while ((opt = getopt(argc, argv, "p:s:rdh")) != -1){
		switch (opt){
		case 'p': profile_arg = optarg; break;
		case 's': settings_arg = optarg; break;
		case 'r': raw_mode = 1; break;
		case 'd': config_print_settings(stdout); return 0;
		default: usage(argv[0]); return opt != 'h';
		}
	}
	/* settings first so a profile's "settings" block wins */
	if (config_load_settings(settings_arg) || (profile_arg && config_load_profile(profile_arg)))
		return 1;
	if (cfg.bash_window_ms > 500){
		fprintf(stderr, "bash_window_ms can be at most 500\n");
		return 1;
	}

	char device[64];
	if (optind < argc)
		snprintf(device, sizeof(device), "%s", argv[optind]);
	else if (board_find(device, sizeof(device))){
		fprintf(stderr, "Can't find the balance board. Is it connected? (see docs/bluetooth.md)\n");
		return 1;
	}
	int fdr = open(device, O_RDONLY);
	if (fdr < 0){
		perror(device);
		return 1;
	}
	fprintf(stderr, "board %s\n", device);

	if (raw_mode)
		return raw(fdr);

	if (output_open())
		return 1;
	fprintf(stderr, "profile %s (%s)\n", prof.name, prof.device == DEV_XBOX ? "xbox pad" : "keyboard/mouse");

	struct input_event ev;
	while (read(fdr, &ev, sizeof(ev)) == sizeof(ev)){
		if (ev.type == EV_ABS)
			board_update(ev.code, ev.value);

		if (ev.type == EV_SYN && ev.code == SYN_REPORT){
			bash_update();
			int x, y;
			board_stick(&x, &y);
			if (output_stick(x, y))
				break;
		}

		/* board button: toggle lean on press, or hold a key/button for as long as it's held */
		if (ev.type == EV_KEY && ev.code == BTN_A && ev.value <= 1)
			output_action(&prof.button, ev.value);
	}
	fprintf(stderr, "board disconnected\n");
	output_close();
	return 1;
}
