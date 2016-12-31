#ifdef __SSE2__
#include <emmintrin.h>
#include <strings.h>
#endif

// the longest common prefix
static inline size_t lcp(const char *s1, size_t len1,
			 const char *s2, size_t len2)
{
    const char *s0 = s1;
#ifdef __SSE2__
    size_t minlen = len1 < len2 ? len1 : len2;
    size_t xmmsize = minlen / 16;
    for (size_t i = 0; i < xmmsize; i++) {
	__m128i xmm1 = _mm_loadu_si128((__m128i *) s1);
	__m128i xmm2 = _mm_loadu_si128((__m128i *) s2);
	xmm1 = _mm_cmpeq_epi8(xmm1, xmm2);
	unsigned mask = _mm_movemask_epi8(xmm1);
	if (mask != 0xffff) {
	    s1 += ffs(~mask) - 1;
	    return s1 - s0;
	}
	s1 += 16, s2 += 16;
    }
#endif
    // when lengths are different, there's no need to test for EOL
    if (len1 != len2)
	while (*s1 == *s2)
	    s1++, s2++;
    else
	while (*s1 && *s1 == *s2)
	    s1++, s2++;
    return s1 - s0;
}
