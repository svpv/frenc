AM_CFLAGS = -Wall -Wextra

lib_LTLIBRARIES = libfrenc.la
libfrenc_la_SOURCES = frenc.c frdec.c

include_HEADERS = frenc.h

otherincludedir = $(includedir)/frenc
otherinclude_HEADERS = lcp.h

bin_PROGRAMS = frenc
frenc_SOURCES = main.c
frenc_LDADD = libfrenc.la
frenc_LDFLAGS = -static

check: frenc
	perl -E 'say "x" x (1<<16); say "x" x (1<<17); say "y"' >in
	./frenc <in >out-enc
	./frenc -d <out-enc >out-dec
	cmp in out-dec
