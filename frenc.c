#include <stdlib.h>
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
	    if (pass == 1) {
		total += 3;
		if (diff > DIFF3_HI)
		    len = olen + DIFF3_HI;
		else if (diff < DIFF3_LO)
		    len = 0;
	    }
	    else {
		*enc++ = -128;
		if (diff > 0) {
		    if (diff > DIFF3_HI) {
			len = olen + DIFF3_HI;
			diff = 32767;
		    }
		    else {
			diff -= DIFF2_HI+1;
			assert(diff <= 32767);
		    }
		}
		else {
		    if (diff < DIFF3_LO) {
			len = 0;
			diff = -32768;
		    }
		    else {
			diff -= DIFF2_LO-1;
			assert(diff >= -32767);
		    }
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
