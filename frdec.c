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

#define UNLIKELY(cond) __builtin_expect(cond, 0)

#define CKBAD(cond)			\
    do {				\
	if (check && UNLIKELY(cond))	\
	    return FRENC_ERR_DATA;	\
    } while (0)

#define APPLY_NEGATIVE_DIFF(diff)	\
    do {				\
	size_t absdiff = -diff;		\
	CKBAD(absdiff > olen);		\
	len = olen - absdiff;		\
    } while (0)

#define APPLY_NONNEGATIVE_DIFF(diff)	\
    do {				\
	len = olen + diff;		\
	CKBAD(len > INT_MAX);		\
    } while (0)

static inline size_t decpass(const char *enc, const char *end,
			     char **v, unsigned *ll, bool hasll,
			     char *strtab, size_t *strtab_size,
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
	if (hasll)
	    *ll++ = strtab - ostrtab - 1;
    }
    size_t olen = 0;
    while (enc < end) {
	int diff = *enc++;
	size_t len;
	if (UNLIKELY(bigdiff[(unsigned char) diff])) {
	    int left = end - enc;
	    if (diff == 127) {
		CKBAD(left < 2);
		diff += (unsigned char) *enc++;
		APPLY_NONNEGATIVE_DIFF(diff);
	    }
	    else if (diff == -127) {
		CKBAD(left < 2);
		diff -= (unsigned char) *enc++;
		APPLY_NEGATIVE_DIFF(diff);
	    }
	    else {
		assert(diff == -128);
		CKBAD(left < 3);
		union { short s16; unsigned short u16; } u;
		memcpy(&u, enc, 2);
		enc += 2;
		u.u16 = le16toh(u.u16);
		if (u.s16 >= 0) {
		    diff = u.s16 + (DIFF2_HI+1);
		    APPLY_NONNEGATIVE_DIFF(diff);
		}
		else {
		    diff = u.s16 + (DIFF2_LO-1);
		    if (diff < DIFF3_LO)
			len = 0;
		    else
			APPLY_NEGATIVE_DIFF(diff);
		}
	    }
	}
	else {
	    CKBAD(enc == end);
	    if (diff < 0)
		APPLY_NEGATIVE_DIFF(diff);
	    else
		APPLY_NONNEGATIVE_DIFF(diff);
	}
	olen = len;
	if (pass == 1)
	    n++;
	// prefix
	if (pass == 1)
	    *strtab_size += len;
	else {
	    *v++ = memcpy(strtab, ostrtab, len);
	    ostrtab = strtab;
	    strtab += len;
	    if (hasll)
		*ll = len;
	}
	// suffix
	if (pass == 1) {
	    len = strlen(enc);
	    *strtab_size += len + 1;
	}
	else {
	    len = stpcpy(strtab, enc) - strtab;
	    strtab += len + 1;
	    if (hasll)
		*ll++ += len;
	}
	enc += len + 1;
    }
    if (pass == 1)
	return n;
    *v = NULL;
    return v - v0;
}

static inline size_t frdecll(const void *enc, size_t encsize, char ***vp,
			     unsigned **llp, bool hasllp)
{
    assert(encsize > 0);
    assert(enc);
    assert(vp);
    const char *end = (char *) enc + encsize;
    if (end[-1] != '\0')
	return FRENC_ERR_DATA;
    // first pass, compute n and the total size
    size_t strtab_size;
    size_t n = decpass(enc, end, NULL, NULL, 0, NULL, &strtab_size, 1, 1);
    if (n >= FRENC_ERROR)
	return n;
    size_t malloc_size = (n + 1) * sizeof(char *) + strtab_size +
			 (hasllp ? n * sizeof *llp : 0);
    char **v = malloc(malloc_size);
    if (v == NULL)
	return FRENC_ERR_MALLOC;
    unsigned *ll =  hasllp ? (void *) (v + n + 1) : NULL;
    char *strtab = !hasllp ? (char *) (v + n + 1) : (char *) (ll + n);
    // second pass, build the output
    decpass(enc, end, v, ll, hasllp, strtab, &strtab_size, 2, 0);
    *vp = v;
    if (hasllp)
	*llp = ll;
    return n;
}

size_t frdec(const void *enc, size_t encsize, char ***vp)
{
    return frdecll(enc, encsize, vp, NULL, 0);
}

size_t frdecl(const void *enc, size_t encsize, char ***vp, unsigned **llp)
{
    return frdecll(enc, encsize, vp, llp, 1);
}
