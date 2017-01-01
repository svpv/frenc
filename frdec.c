#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
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
