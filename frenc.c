#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <endian.h>
#include "frenc.h"
#include "lcp.h"

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

size_t frenc(char **v, size_t n, void **encp)
{
    assert(n > 0);
    // first pass, compute the total size
    size_t len1 = strlen(v[0]);
    size_t total = len1 + 1;
    // preprocessing, improves speed but takes more memory
    int *pplen = malloc(n * sizeof(int));
    if (pplen == NULL)
	return FRENC_ERR_MALLOC;
    size_t olen = 0;
    for (size_t i = 1; i < n; i++) {
	size_t len2 = strlen(v[i]);
	size_t len = lcp(v[i-1], len1, v[i], len2);
	if (len > INT_MAX) {
	    free(pplen);
	    return FRENC_ERR_RANGE;
	}
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
		return FRENC_ERR_RANGE;
	    }
	    total += 3;
	}
	total += len2 - len + 1;
	olen = len;
	len1 = len2;
    }
    if (encp == NULL) {
	free(pplen);
	return total;
    }
    // second pass, build the output
    char *out = malloc(total);
    if (out == NULL) {
	free(pplen);
	return FRENC_ERR_MALLOC;
    }
    char *out0 = *encp = out;
    out = stpcpy(out, v[0]) + 1;
    olen = 0;
    for (size_t i = 1; i < n; i++) {
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
    assert(out - out0 == (ptrdiff_t) total);
    return total;
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

size_t frdec(const void *enc0, size_t encsize, char ***vp)
{
    assert(encsize > 0);
    const char *enc = enc0;
    if (enc[encsize-1] != '\0')
	return FRENC_ERR_DATA;
    // first pass, compute n and the total size
    const char *end = enc + encsize;
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
		    return FRENC_ERR_DATA;
		diff += (unsigned char) *enc++;
	    }
	    else if (diff == -127) {
		if (left < 1)
		    return FRENC_ERR_DATA;
		diff -= (unsigned char) *enc++;
	    }
	    else {
		assert(diff == -128);
		if (left < 2)
		    return FRENC_ERR_DATA;
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
    if (vp == NULL)
	return n;
    // second pass, build the output
    char **strv = *vp = malloc(total);
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
		if (left < 2)
		    return FRENC_ERR_DATA;
		diff += (unsigned char) *enc++;
	    }
	    else if (diff == -127) {
		if (left < 2)
		    return FRENC_ERR_DATA;
		diff -= (unsigned char) *enc++;
	    }
	    else {
		assert(diff == -128);
		if (left < 3)
		    return FRENC_ERR_DATA;
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
	else if (enc == end)
	    return FRENC_ERR_DATA;
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
    assert(strtab - (char *) *vp == (ptrdiff_t) total);
    *strv = NULL;
    return n;
}
