#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "frenc.h"

// This main routine is only provided for testing purposes, and also
// to profile frenc() and frdec() routines.  It is probably inefficient
// on big inputs, for which a streaming API should rather be used.
int main(int argc, char **argv)
{
    bool dec = 0;
    int opt;
    while ((opt = getopt(argc, argv, "d")) != -1) {
	switch (opt) {
	case 'd':
	    dec = 1;
	    break;
	default:
	    goto usage;
	}
    }
#define progname argv[0]
    if (argc > optind + 1) {
	fprintf(stderr, "%s: too many arguments\n", progname);
usage:	fprintf(stderr, "Usage: %s [-d] [file]\n", progname);
	return 1;
    }
    if (argc > optind && strcmp(argv[optind], "-") != 0) {
	stdin = freopen(argv[optind], "r", stdin);
	if (!stdin) {
	    fprintf(stderr, "%s: cannot open %s\n", progname, argv[optind]);
	    return 1;
	}
    }
    size_t n = 0;
    char **v = NULL;
    // decode
    if (dec) {
	char *buf = NULL;
	size_t size = 0;
	while (1) {
	    buf = realloc(buf, size + BUFSIZ);
	    assert(buf);
	    size_t m = fread(buf + size, 1, BUFSIZ, stdin);
	    size += m;
	    if (m < BUFSIZ)
		break;
	}
	if (size == 0) {
empty:	    fprintf(stderr, "%s: empty input\n", progname);
	    return 1;
	}
	unsigned *lens;
	n = frdecl(buf, size, &v, &lens);
	free(buf);
	if (n >= FRENC_ERROR) {
	    fprintf(stderr, "%s: frdec failed\n", progname);
	    return 1;
	}
	assert(v);
	for (size_t i = 0; i < n; i++) {
	    v[i][lens[i]] = '\n';
	    fwrite(v[i], lens[i] + 1, 1, stdout);
	}
	return 0;
    }
    // encode
    char *line = NULL;
    size_t alloc_size = 0;
    ssize_t len;
    while ((len = getline(&line, &alloc_size, stdin)) >= 0) {
	if (len > 0 && line[len-1] == '\n')
	    line[--len] = '\0';
	if ((n % 1024) == 0)
	    v = realloc(v, sizeof(*v) * (n + 1024));
	assert(v);
	v[n++] = line;
	line = NULL;
	alloc_size = 0;
    }
    if (n == 0)
	goto empty;
    void *enc;
    size_t size = frenc(v, n, &enc);
    if (size >= FRENC_ERROR) {
	fprintf(stderr, "%s: frenc failed\n", progname);
	return 1;
    }
    fwrite(enc, size, 1, stdout);
    free(enc);
    return 0;
}
