// This library implements an improved variant of front compression
// for sorted strings (also known as incremental encoding).
// The encoding is similar, but not identical to, the one described
// in locatedb(5), to which the reader may refer.

#ifndef FRENC_H
#define FRENC_H

// Encode an argv[]-like array of C strings v[] with the number of
// elements n > 0 (unlike argv[], v[n] need not be a NULL sentinel).
// This function does not modify neither v[] nor its strings (but
// adding const qualifiers to announce these facts may turn out
// to be confusing).
// Returns the compressed size > 0, or an error (see below).
// Upon success, the compressed data is returned via the encp pointer.
// The pointer will point to a malloc'd buffer which the caller
// should free after use.
size_t frenc(char **v, size_t n, void **encp);

// Decode the compressed data (enc) whose size is encsize > 0.
// Returns the number n of the decoded C strings, or an error.
// Upon success, the array of strings v[] is returned via the vp
// pointer.  The array will have a NULL sentinel installed at v[n].
// The array along with the strings is allocated in a single malloc
// chunk (i.e. the strings will be placed after v[n]); the caller
// should free it after use.
size_t frdec(const void *enc, size_t encsize, char ***vp);

// The most obvious reason for an error is a malloc failure.
#define FRENC_ERR_MALLOC (~(size_t)0-0)
// There are also certain size limits: each string in v[] must be
// shorter than 2G; the total number of strings cannot exceed 255G,
// and the encoded size is also limited to O(255G).  By today's
// perceptions (as of late 2016), it seems very unlikely that anyone
// will ever hit these limits (on 32-bit platforms, other limits also
// apply which are obviously much tighter).  But, "whatever you may
// say, these things do happen - rarely, I admit, but they do happen".
#define FRENC_ERR_RANGE  (~(size_t)0-1)
// During decoding, malformed data can be encountered.
// NB: the data cannot be empty, though; i.e. the size cannot be zero.
// In frenc and frdec, size=0 will simply trigger an assertion failure.
#define FRENC_ERR_DATA   (~(size_t)0-2)
// In the streaming API, I/O errors can happen.  This does not include
// data shortage (i.e. EOF), which is regarded as an invalid input.
// Further information about a particular I/O error can be obtained
// with ferror(3).
#define FRENC_ERR_STDIO  (~(size_t)0-3)
// The caller should test for an error with (ret >= FRENC_ERROR).
#define FRENC_ERROR      (~(size_t)0-7)

// The stdio(3)-based streaming API.
#ifdef FRENC_STDIO
// These functions encode (frencio) or decode (frdecio) the input
// stream to the output stream.  The z flag indicates whether the '\0'
// line delimiter should be used instead of '\n' (akin to "sort -z").
// These functions return the total number of lines processed,
// or an error.  As a special case, 0 is returned if the input stream
// is empty; in this case, nothing is written to the output (the caller
// should probably treat this as an error).  As another special case,
// frencio() returns FRENC_ERR_DATA when z=false and an input line
// contains an embedded '\0' byte.
size_t frencio(FILE *in, FILE *out, bool z);
size_t frdecio(FILE *in, FILE *out, bool z);
#endif

#endif
