all: bfb

bfb: bfb.c bfb.h
	gcc -o bfb bfb.c -g3 -fsanitize=address

.PHONY:

clean:
	-rm bfb

