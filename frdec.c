#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <endian.h>
#define FRENC_FORMAT
#include "frenc.h"

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

static inline size_t decpass(const char *enc, const char *end,
			     char **v, char *strtab, size_t *strtab_size,
			     int pass, bool check)
{
    char **v0;
    size_t n;
    char *ostrtab;
    if (pass == 1) {
	n = 1;
	*strtab_size = strlen(enc) + 1;
	enc += *strtab_size;
    }
    else {
	v0 = v;
	*v++ = ostrtab = strtab;
	strtab = stpcpy(strtab, enc) + 1;
	enc += strtab - ostrtab;
    }
    size_t olen = 0;
    while (enc < end) {
	int diff = *enc++;
	if (bigdiff[(unsigned char) diff]) {
	    int left = end - enc;
	    if (diff == 127) {
		if (check && left < 2)
		    return FRENC_ERR_DATA;
		diff += (unsigned char) *enc++;
	    }
	    else if (diff == -127) {
		if (check && left < 2)
		    return FRENC_ERR_DATA;
		diff -= (unsigned char) *enc++;
	    }
	    else {
		assert(diff == -128);
		if (check && left < 3)
		    return FRENC_ERR_DATA;
		union { short s16; unsigned short u16; } u;
		memcpy(&u, enc, 2);
		enc += 2;
		u.u16 = le16toh(u.u16);
		if (u.s16 >= 0)
		    diff = u.s16 + (DIFF2_HI+1);
		else
		    diff = u.s16 + (DIFF2_LO-1);
	    }
	}
	else if (check && enc == end)
	    return FRENC_ERR_DATA;
	if (pass == 1)
	    n++;
	// prefix
	size_t len;
	if (diff < 0) {
	    size_t absdiff = -diff;
	    if (absdiff > olen)
		return FRENC_ERR_DATA;
	    len = olen - absdiff;
	}
	else {
	    len = olen + diff;
	    if (len > INT_MAX)
		return FRENC_ERR_DATA;
	}
	olen = len;
	if (pass == 1)
	    *strtab_size += len;
	else {
	    *v++ = memcpy(strtab, ostrtab, len);
	    ostrtab = strtab;
	    strtab += len;
	}
	// suffix
	if (pass == 1) {
	    len = strlen(enc);
	    *strtab_size += len + 1;
	}
	else {
	    len = stpcpy(strtab, enc) - strtab;
	    strtab += len + 1;
	}
	enc += len + 1;
    }
    if (pass == 1)
	return n;
    *v = NULL;
    return v - v0;
}

size_t frdec(const void *enc, size_t encsize, char ***vp)
{
    assert(encsize > 0);
    assert(enc);
    assert(vp);
    const char *end = (char *) enc + encsize;
    if (end[-1] != '\0')
	return FRENC_ERR_DATA;
    // first pass, compute n and the total size
    size_t strtab_size;
    size_t n = decpass(enc, end, NULL, NULL, &strtab_size, 1, 1);
    if (n >= FRENC_ERROR)
	return n;
    size_t malloc_size = (n + 1) * sizeof(char *) + strtab_size;
    char **v = malloc(malloc_size);
    if (v == NULL)
	return FRENC_ERR_MALLOC;
    char *strtab = (char *) (v + n + 1);
    // second pass, build the output
    decpass(enc, end, v, strtab, &strtab_size, 2, 0);
    *vp = v;
    return n;
}
