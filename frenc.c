#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <endian.h>
#define FRENC_FORMAT
#include "frenc.h"
#include "lcp.h"

// It is hard to estimate the encoded size from n only.  Therefore,
// the encoding is done in two passes: on the first pass, the encoded
// size is calculated, and on the second, the actual encoding is done.
// Also, lcp values obtained on the first pass are stored in pplen[]
// and reused on the second ("pp" stands for preprocessing).
static inline size_t encpass(char **v, size_t n, char *enc,
			     int pass, int *pplen)
{
    // len1 and len2 are only used in the first pass to calculate
    // common prefix lengths, which are stored in pplen[]; in the
    // second pass, only stpcpy(3) is used
    size_t len1;
    if (pass == 1)
	len1 = strlen(v[0]);
    else
	enc = stpcpy(enc, v[0]) + 1;
    // total encoded size is only calculated in the first pass
    size_t total = 0;
    if (pass == 1)
	total = len1 + 1;
    size_t olen = 0;
    for (size_t i = 1; i < n; i++) {
	size_t len, len2;
	if (pass == 2)
	    len = pplen[i];
	else {
	    len2 = strlen(v[i]);
	    len = lcp(v[i-1], len1, v[i], len2);
	    if (len > INT_MAX)
		return FRENC_ERR_RANGE;
	    pplen[i] = len;
	}
	int diff = (int) len - (int) olen;
	if (diff >= DIFF1_LO && diff <= DIFF1_HI) {
	    if (pass == 1)
		total += 1;
	    else
		*enc++ = diff;
	}
	else if (diff >= DIFF2_LO && diff <= DIFF2_HI) {
	    if (pass == 1)
		total += 2;
	    else {
		if (diff > 0) {
		    *enc++ = 127;
		    diff -= 127;
		    assert(diff >= 0);
		}
		else {
		    *enc++ = -127;
		    diff += 127;
		    assert(diff <= 255);
		}
		unsigned char diff8 = diff;
		memcpy(enc++, &diff8, 1);
	    }
	}
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
	    if (pass == 1)
		total += 3;
	    else {
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
		memcpy(enc, &u, 2);
		enc += 2;
	    }
	}
	if (pass == 2)
	    enc = stpcpy(enc, v[i] + len) + 1;
	else {
	    total += len2 - len + 1;
	    len1 = len2;
	}
	olen = len;
    }
    return total;
}

size_t frenc(char **v, size_t n, void **encp)
{
    assert(n > 0);
    assert(v);
    assert(encp);
    int *pplen = malloc(n * sizeof(int));
    if (pplen == NULL)
	return FRENC_ERR_MALLOC;
    // first pass
    size_t total = encpass(v, n, NULL, 1, pplen);
    if (total >= FRENC_ERROR) {
	free(pplen);
	return total;
    }
    // second pass, build the output
    char *enc = malloc(total);
    if (enc == NULL) {
	free(pplen);
	return FRENC_ERR_MALLOC;
    }
    encpass(v, n, enc, 2, pplen);
    free(pplen);
    *encp = enc;
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
