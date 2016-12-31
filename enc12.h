// This implements 12-bits packed integers with 7-bit mantissa,
// which cover the range [0,255G].  Such integers can be used
// e.g. to store size hints for malloc.
static inline unsigned enc12(size_t n)
{
    // One good thing about this encoding is that, due to the "denormal"
    // thing, small numbers stand for themselves.  And another good thing
    // is that the numbers are monotonically increasing.
    if (n <= 256)
	return n;
    // The exponent is off by one, because e=0 actually indicates
    // denormalized representation: 0xxxxxxx.
    size_t e = 1;
    // Loop: while the number 1xxxxxxx (1 indicates the most significant bit)
    // does not fit into 8 bits, shift it into its proper "rayon" (region),
    // meanwhile increasing the exponent.
    do {
	// round up
	n = n / 2 + n % 2;
	e++;
    } while (n >= 256);
    // The implicit MSB is now taken off.  0177 means "7 bits".  Recall that
    // 7 means 3 bits, such as in Unix permissions, and 1 means, well, 1 bit.
    n &= 0177;
    // 7-bit mantissa and 5-bit exponent are now ready.  The caller may want
    // to check that the result is less than 4096.  If the result is >= 4096,
    // the original number is > 255G and cannot be represented with 12 bits.
    // This is only possible with 64-bit size_t.
    return (e << 7) | n;
}

// Yet another good thing about this encoding is that it is reversible -
// size hints can actually be decoded into the rounded-up numbers,
// with the average overhead below 0.3%.
static inline size_t dec12(unsigned h)
{
    // denormal
    if (h <= 256)
	return h;
    // 7-bit mantissa with the implicit high bit set
    size_t n = ((h &  0177) + 128);
    // the exponent, off by one
    size_t e = ((h & ~0177) >> 07) - 1;
    // now, is this result valid?
    return n << e;
}

// So the result is always valid when a hint is decoded into a 64-bit size_t.
// Thus the caller should test for !(sizeof(size_t) < 5 && h > ENC12_MAX32)
// before ever calling dec12.
#define ENC12_MAX32 3327
