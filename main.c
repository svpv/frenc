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
    int n = 0;
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
	n = frdec(buf, size - 1, &v);
	free(buf);
	if (n < 0) {
	    fprintf(stderr, "%s: frdec failed\n", progname);
	    return 1;
	}
	assert(v);
	for (int i = 0; i < n; i++)
	    puts(v[i]);
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
    char *enc;
    int enclen = frenc(v, n, &enc);
    if (enclen < 0) {
	fprintf(stderr, "%s: frenc failed\n", progname);
	return 1;
    }
    fwrite(enc, enclen + 1, 1, stdout);
    free(enc);
    return 0;
}
