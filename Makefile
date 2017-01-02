CFLAGS = -g -O2 -Wall -Wextra
frenc: frenc.c frdec.c main.c

check: frenc
	perl -E 'say "x" x (1<<16); say "x" x (1<<17); say "y"' >in
	./frenc <in >out-enc
	./frenc -d <out-enc >out-dec
	cmp in out-dec
