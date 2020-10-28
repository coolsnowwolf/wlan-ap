/* SPDX-License-Identifier: BSD-3-Clause */

#include <stdio.h>
#include <string.h>
#include "websocket/private-libwebsockets.h"

static const char encode[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			     "abcdefghijklmnopqrstuvwxyz0123456789+/";
static const char decode[] = "|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW"
			     "$$$$$$XYZ[\\]^_`abcdefghijklmnopq";

LWS_VISIBLE int
lws_b64_encode_string(const char *in, int in_len, char *out, int out_size)
{
	unsigned char triple[3];
	int i;
	int len;
	int line = 0;
	int done = 0;

	while (in_len) {
		len = 0;
		for (i = 0; i < 3; i++) {
			if (in_len) {
				triple[i] = *in++;
				len++;
				in_len--;
			} else
				triple[i] = 0;
		}

		if (done + 4 >= out_size)
			return -1;

		*out++ = encode[triple[0] >> 2];
		*out++ = encode[((triple[0] & 0x03) << 4) |
					     ((triple[1] & 0xf0) >> 4)];
		*out++ = (len > 1 ? encode[((triple[1] & 0x0f) << 2) |
					     ((triple[2] & 0xc0) >> 6)] : '=');
		*out++ = (len > 2 ? encode[triple[2] & 0x3f] : '=');

		done += 4;
		line += 4;
	}

	if (done + 1 >= out_size)
		return -1;

	*out++ = '\0';

	return done;
}

/*
 * returns length of decoded string in out, or -1 if out was too small
 * according to out_size
 */

LWS_VISIBLE int
lws_b64_decode_string(const char *in, char *out, int out_size)
{
	int len, i, c = 0, done = 0;
	unsigned char v, quad[4];

	while (*in) {

		len = 0;
		for (i = 0; i < 4 && *in; i++) {

			v = 0;
			c = 0;
			while (*in && !v) {
				c = v = *in++;
				v = (v < 43 || v > 122) ? 0 : decode[v - 43];
				if (v)
					v = (v == '$') ? 0 : v - 61;
			}
			if (c) {
				len++;
				if (v)
					quad[i] = v - 1;
			} else
				quad[i] = 0;
		}

		if (out_size < (done + len - 1))
			/* out buffer is too small */
			return -1;

		/*
		 * "The '==' sequence indicates that the last group contained
		 * only one byte, and '=' indicates that it contained two
		 * bytes." (wikipedia)
		 */

		if (!*in && c == '=')
			len--;

		if (len >= 2)
			*out++ = quad[0] << 2 | quad[1] >> 4;
		if (len >= 3)
			*out++ = quad[1] << 4 | quad[2] >> 2;
		if (len >= 4)
			*out++ = ((quad[2] << 6) & 0xc0) | quad[3];

		done += len - 1;
	}

	if (done + 1 >= out_size)
		return -1;

	*out = '\0';

	return done;
}

#if 0
int
lws_b64_selftest(void)
{
	char buf[64];
	unsigned int n,  r = 0;
	unsigned int test;
	/* examples from https://en.wikipedia.org/wiki/Base64 */
	static const char * const plaintext[] = {
		"any carnal pleasure.",
		"any carnal pleasure",
		"any carnal pleasur",
		"any carnal pleasu",
		"any carnal pleas",
		"Admin:kloikloi"
	};
	static const char * const coded[] = {
		"YW55IGNhcm5hbCBwbGVhc3VyZS4=",
		"YW55IGNhcm5hbCBwbGVhc3VyZQ==",
		"YW55IGNhcm5hbCBwbGVhc3Vy",
		"YW55IGNhcm5hbCBwbGVhc3U=",
		"YW55IGNhcm5hbCBwbGVhcw==",
		"QWRtaW46a2xvaWtsb2k="
	};

	for (test = 0; test < sizeof plaintext / sizeof(plaintext[0]); test++) {

		buf[sizeof(buf) - 1] = '\0';
		n = lws_b64_encode_string(plaintext[test],
				      strlen(plaintext[test]), buf, sizeof buf);
		if (n != strlen(coded[test]) || strcmp(buf, coded[test])) {
			lwsl_err("Failed lws_b64 encode selftest "
					   "%d result '%s' %d\n", test, buf, n);
			r = -1;
		}

		buf[sizeof(buf) - 1] = '\0';
		n = lws_b64_decode_string(coded[test], buf, sizeof buf);
		if (n != strlen(plaintext[test]) ||
						 strcmp(buf, plaintext[test])) {
			lwsl_err("Failed lws_b64 decode selftest "
				 "%d result '%s' / '%s', %d / %d\n",
				 test, buf, plaintext[test], n, strlen(plaintext[test]));
			r = -1;
		}
	}

	lwsl_notice("Base 64 selftests passed\n");

	return r;
}
#endif