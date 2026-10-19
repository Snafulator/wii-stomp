/* SPDX-License-Identifier: GPL-3.0-only */
#include <errno.h>
#include <fcntl.h>
#include <linux/uinput.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "output.h"

int lean_on = 1;
static int pad = -1, kbm = -1; /* uinput fds; -1 = not created */

/* writes a batch of events; the caller ends it with a SYN_REPORT */
static int sendEvents(int fd, const struct input_event *evs, int n)
{
	ssize_t want = n * sizeof(*evs);
	if (write(fd, evs, want) != want){
		perror("writing to virtual device");
		return -1;
	}
	return 0;
}

static int openUinput(void)
{
	int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
	if (fd < 0)
		perror("/dev/uinput (see Permissions in README.md)");
	return fd;
}

/* registers the device under name/id once its capabilities are set */
static int createDevice(int fd, const char *name, struct input_id id)
{
	struct uinput_setup u;
	memset(&u, 0, sizeof(u));
	snprintf(u.name, sizeof(u.name), "%s", name);
	u.id = id;
	if (ioctl(fd, UI_DEV_SETUP, &u) < 0 || ioctl(fd, UI_DEV_CREATE) < 0){
		fprintf(stderr, "creating '%s': %s\n", name, strerror(errno));
		return -1;
	}
	return 0;
}

/* A virtual Xbox 360 pad, with the same buttons, axes and ids as the xpad
 * driver, so Steam and SDL apply their built-in mapping and Steam Input can
 * remap it. Only the left stick (lean) and the profile's buttons are used. */
static const int padButtons[] = {
	BTN_A, BTN_B, BTN_X, BTN_Y, BTN_TL, BTN_TR,
	BTN_SELECT, BTN_START, BTN_MODE, BTN_THUMBL, BTN_THUMBR,
};
static const struct { int code, lo, hi; } padAxes[] = {
	{ ABS_X, -AXIS_MAX, AXIS_MAX }, { ABS_Y, -AXIS_MAX, AXIS_MAX },  /* left stick: lean */
	{ ABS_RX, -32768, 32767 }, { ABS_RY, -32768, 32767 },            /* right stick */
	{ ABS_Z, 0, 255 }, { ABS_RZ, 0, 255 },                           /* triggers */
	{ ABS_HAT0X, -1, 1 }, { ABS_HAT0Y, -1, 1 },                      /* d-pad */
};

static int openPad(void)
{
	if ((pad = openUinput()) < 0)
		return -1;
	ioctl(pad, UI_SET_EVBIT, EV_KEY);
	for (unsigned i = 0; i < sizeof(padButtons) / sizeof(padButtons[0]); i++)
		ioctl(pad, UI_SET_KEYBIT, padButtons[i]);

	ioctl(pad, UI_SET_EVBIT, EV_ABS);
	for (unsigned i = 0; i < sizeof(padAxes) / sizeof(padAxes[0]); i++){
		struct uinput_abs_setup axis;
		memset(&axis, 0, sizeof(axis));
		axis.code = padAxes[i].code;
		axis.absinfo.minimum = padAxes[i].lo;
		axis.absinfo.maximum = padAxes[i].hi;
		if (ioctl(pad, UI_SET_ABSBIT, axis.code) < 0 || ioctl(pad, UI_ABS_SETUP, &axis) < 0){
			perror("setting up pad axis");
			return -1;
		}
	}

	struct input_id xpad = { .bustype = BUS_USB, .vendor = 0x045e, .product = 0x028e, .version = 0x0114 };
	return createDevice(pad, "Microsoft X-Box 360 pad", xpad);
}

/* A virtual keyboard + mouse with every key a profile can name. It has
 * relative X/Y axes (never moved) so desktops treat it as a mouse and
 * accept its mouse buttons. */
static int openKbm(void)
{
	if ((kbm = openUinput()) < 0)
		return -1;
	ioctl(kbm, UI_SET_EVBIT, EV_KEY);
	for (int i = 0; i < nkeynames; i++)
		if (!is_pad_button(keynames[i].code))
			ioctl(kbm, UI_SET_KEYBIT, keynames[i].code);
	ioctl(kbm, UI_SET_EVBIT, EV_REL);
	ioctl(kbm, UI_SET_RELBIT, REL_X);
	ioctl(kbm, UI_SET_RELBIT, REL_Y);

	struct input_id id = { .bustype = BUS_VIRTUAL, .vendor = 0x3, .product = 0x4, .version = 1 };
	return createDevice(kbm, "wii-stomp keys", id);
}

int output_open(void)
{
	struct action *acts[] = { &prof.bash_l, &prof.bash_r, &prof.button };
	lean_on = prof.lean;
	if (prof.device == DEV_XBOX ? openPad() : openKbm())
		return -1;
	for (int i = 0; i < 3; i++)
		acts[i]->fd = prof.device == DEV_XBOX ? pad : kbm;
	return 0;
}

void output_action(struct action *a, int value)
{
	if (a->type == ACT_TOGGLE_LEAN && value == 1){
		lean_on = !lean_on;
		fprintf(stderr, "movement %s\n", lean_on ? "on" : "off");
	} else if (a->type == ACT_PRESS){
		struct input_event evs[] = {
			{ .type = EV_KEY, .code = a->code, .value = value },
			{ .type = EV_SYN, .code = SYN_REPORT },
		};
		sendEvents(a->fd, evs, 2);
	}
}

int output_stick(int x, int y)
{
	if (pad < 0)
		return 0;
	struct input_event evs[] = {
		{ .type = EV_ABS, .code = ABS_X, .value = lean_on ? x : 0 },
		{ .type = EV_ABS, .code = ABS_Y, .value = lean_on ? y : 0 },
		{ .type = EV_SYN, .code = SYN_REPORT },
	};
	return sendEvents(pad, evs, 3);
}

void output_close(void)
{
	/* closing a uinput fd removes its device; destroy explicitly anyway */
	int *fds[] = { &pad, &kbm };
	for (int i = 0; i < 2; i++)
		if (*fds[i] >= 0){
			ioctl(*fds[i], UI_DEV_DESTROY);
			close(*fds[i]);
			*fds[i] = -1;
		}
}
