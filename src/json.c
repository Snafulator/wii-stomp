/* SPDX-License-Identifier: GPL-3.0-only */
#include <ctype.h>
#include <string.h>

#include "json.h"

static struct json_kv *kv;
static int nkv, maxkv;
static const char *jp;

static void jws(void)
{
	while (*jp == ' ' || *jp == '\t' || *jp == '\n' || *jp == '\r') jp++;
}

static int jstring(char *out, int n)
{
	if (*jp != '"') return -1;
	jp++;
	int i = 0;
	while (*jp && *jp != '"'){
		if (*jp == '\\' && jp[1]) jp++;
		if (i < n - 1) out[i++] = *jp;
		jp++;
	}
	out[i] = 0;
	if (*jp != '"') return -1;
	jp++;
	return 0;
}

static int jobject(const char *prefix);

static int jvalue(const char *key)
{
	jws();
	if (*jp == '{') return jobject(key);
	if (nkv == maxkv) return -1;
	snprintf(kv[nkv].key, sizeof(kv[nkv].key), "%s", key);
	if (*jp == '"'){
		if (jstring(kv[nkv].val, sizeof(kv[nkv].val))) return -1;
	} else {
		int i = 0;
		while (isalnum((unsigned char)*jp) || *jp == '-' || *jp == '+' || *jp == '.'){
			if (i < (int)sizeof(kv[nkv].val) - 1) kv[nkv].val[i++] = *jp;
			jp++;
		}
		kv[nkv].val[i] = 0;
		if (!i) return -1;
	}
	nkv++;
	return 0;
}

static int jobject(const char *prefix)
{
	jp++; /* '{' */
	jws();
	if (*jp == '}'){ jp++; return 0; }
	while (1){
		char k[64], full[128];
		jws();
		if (jstring(k, sizeof(k))) return -1;
		jws();
		if (*jp != ':') return -1;
		jp++;
		snprintf(full, sizeof(full), "%.60s%s%s", prefix, *prefix ? "." : "", k);
		if (jvalue(full)) return -1;
		jws();
		if (*jp == ','){ jp++; continue; }
		if (*jp == '}'){ jp++; return 0; }
		return -1;
	}
}

int json_read(FILE *f, const char *path, struct json_kv *out, int max)
{
	static char text[65536];
	size_t len = fread(text, 1, sizeof(text) - 1, f);
	text[len] = 0;
	fclose(f);

	kv = out;
	maxkv = max;
	nkv = 0;
	jp = text;
	jws();
	if (*jp != '{' || jobject("")){
		int line = 1;
		for (const char *c = text; c < jp; c++)
			if (*c == '\n') line++;
		fprintf(stderr, "%s:%d: bad JSON\n", path, line);
		return -1;
	}
	return nkv;
}
