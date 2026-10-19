/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef WII_STOMP_JSON_H
#define WII_STOMP_JSON_H

#include <stdio.h>

/* Minimal JSON reader for settings and profiles: objects, strings, true/false/numbers.
 * Nested keys are flattened with dots, e.g. "bash": {"left": ...} -> bash.left */
struct json_kv { char key[128], val[64]; };

/* reads f (and closes it) into kv[]; returns the number of values, or -1
 * after printing "path:line: bad JSON" */
int json_read(FILE *f, const char *path, struct json_kv *kv, int max);

#endif
