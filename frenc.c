#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <endian.h>
#include "frenc.h"

// the longest common prefix
static inline size_t lcp(const char *s1, const char *s2)
{
    const char *s0 = s1;
    while (*s1 && *s1 == *s2)
	s1++, s2++;
    return s1 - s0;
}

// 1-byte diff encodes [-126,126]
#define DIFF1_LO (-126)
#define DIFF1_HI (+126)

// if the first byte is -127 or 127, the second byte is read
// as unsigned addend with the sign assumed from the first byte
#define DIFF2_LO (-127-255)
#define DIFF2_HI (+127+255)

// if the first byte -128, two more bytes are read to define
// the range outside of [DIFF2_LO,DIFF2_HI]
#define DIFF3_LO (DIFF2_LO-1-32768)
#define DIFF3_HI (DIFF2_HI+1+32767)

int frenc(char *v[], int n, char **enc)
{
    assert(n > 0);
    // first pass, compute the total size
    size_t total = strlen(v[0]) + 1;
    // preprocessing, improves speed but takes more memory
    int *pplen = malloc(n * sizeof(int));
    if (pplen == NULL)
	return -2;
    size_t olen = 0;
    for (int i = 1; i < n; i++) {
	size_t len = lcp(v[i-1], v[i]);
	assert(len <= INT_MAX);
	pplen[i] = len;
	int diff = (int) len - (int) olen;
	if (diff >= DIFF1_LO && diff <= DIFF1_HI)
	    total += 1;
	else if (diff >= DIFF2_LO && diff <= DIFF2_HI)
	    total += 2;
	else {
	    if (diff > DIFF3_HI) {
		// this will result in inefficient encoding
		diff = DIFF3_HI;
		len = olen + diff;
	    }
	    else if (diff < DIFF3_LO) {
		// don't know that to do: to keep the encoding reversible,
		// we MUST be able to restore the prefix, but currently we can't
		return -1;
	    }
	    total += 3;
	}
	total += strlen(v[i] + len) + 1;
	olen = len;
    }
    assert(total <= INT_MAX);
    if (enc == NULL) {
	free(pplen);
	return total - 1;
    }
    // second pass, build the output
    char *out = malloc(total);
    if (out == NULL) {
	free(pplen);
	return -2;
    }
    char *out0 = *enc = out;
    out = stpcpy(out, v[0]) + 1;
    olen = 0;
    for (int i = 1; i < n; i++) {
	size_t len = pplen[i];
	int diff = (int) len - (int) olen;
	if (diff >= DIFF1_LO && diff <= DIFF1_HI)
	    *out++ = diff;
	else if (diff >= DIFF2_LO && diff <= DIFF2_HI) {
	    if (diff > 0) {
		*out++ = 127;
		diff -= 127;
		assert(diff >= 0);
	    }
	    else {
		*out++ = -127;
		diff += 127;
		assert(diff <= 255);
	    }
	    unsigned char diff8 = diff;
	    memcpy(out++, &diff8, 1);
	}
	else {
	    if (diff > DIFF3_HI) {
		diff = DIFF3_HI;
		len = olen + diff;
	    }
	    if (diff > 0) {
		diff -= DIFF2_HI+1;
		assert(diff <= 32767);
	    }
	    else {
		diff -= DIFF2_LO-1;
		assert(diff >= -32768);
	    }
	    union { short s16; unsigned short u16; } u = { diff };
	    u.u16 = htole16(u.u16);
	    memcpy(out, &u, 2);
	    out += 2;
	}
	out = stpcpy(out, v[i] + len) + 1;
	olen = len;
    }
    free(pplen);
    assert(out - out0 == (int) total);
    return total - 1;
}

static const bool bigdiff[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

int frdec(const char *enc, int enclen, char ***v)
{
    assert(enclen >= 0);
    assert(enc[enclen] == '\0');
    // first pass, compute n and the total size
    const char *enc0 = enc;
    const char *end = enc + enclen;
    size_t n = 1;
    size_t len = strlen(enc);
    enc += len + 1;
    // additionally v[n] will have NULL sentinel
    size_t total = len + 1 + 2 * sizeof(char *);
    int olen = 0;
    while (enc < end) {
	int diff = *enc++;
	if (bigdiff[(unsigned char) diff]) {
	    int left = end - enc;
	    if (diff == 127) {
		if (left < 1)
		    return -1;
		diff += (unsigned char) *enc++;
	    }
	    else if (diff == -127) {
		if (left < 1)
		    return -1;
		diff -= (unsigned char) *enc++;
	    }
	    else {
		assert(diff == -128);
		if (left < 2)
		    return -2;
		union { short s16; unsigned short u16; } u;
		memcpy(&u, enc, 2);
		enc += 2;
		u.u16 = le16toh(u.u16);
		if (u.s16 >= 0)
		    diff = u.s16 + (DIFF2_HI+1);
		else
		    diff = u.s16 - (DIFF2_LO-1);
	    }
	}
	n++;
	// prefix
	len = olen + diff;
	olen = len;
	total += len;
	// suffix
	len = strlen(enc);
	enc += len + 1;
	total += len + 1 + sizeof(char *);
    }
    assert(n <= INT_MAX);
    assert(total <= INT_MAX);
    if (v == NULL)
	return n;
    // second pass, build the output
    char **strv = *v = malloc(total);
    char *strtab = (char *) (strv + n + 1);
    char *ostrtab = strtab;
    *strv++ = strtab;
    enc = enc0;
    strtab = stpcpy(strtab, enc) + 1;
    enc += strtab - ostrtab;
    olen = 0;
    while (enc < end) {
	int diff = *enc++;
	if (bigdiff[(unsigned char) diff]) {
	    int left = end - enc;
	    if (diff == 127) {
		if (left < 1)
		    return -1;
		diff += (unsigned char) *enc++;
	    }
	    else if (diff == -127) {
		if (left < 1)
		    return -1;
		diff -= (unsigned char) *enc++;
	    }
	    else {
		assert(diff == -128);
		if (left < 2)
		    return -2;
		union { short s16; unsigned short u16; } u;
		memcpy(&u, enc, 2);
		enc += 2;
		u.u16 = le16toh(u.u16);
		if (u.s16 >= 0)
		    diff = u.s16 + (DIFF2_HI+1);
		else
		    diff = u.s16 - (DIFF2_LO-1);
	    }
	}
	// prefix
	len = olen + diff;
	olen = len;
	*strv++ = memcpy(strtab, ostrtab, len);
	ostrtab = strtab;
	strtab += len;
	// suffix
	len = stpcpy(strtab, enc) - strtab;
	enc += len + 1;
	strtab += len + 1;
    }
    assert(strtab - (char *) *v == (int) total);
    *strv = NULL;
    return n;
}
