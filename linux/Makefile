# $scrotwm$ 

CFLAGS+= -Wall -ggdb -D_GNU_SOURCE -I.
LDADD+= -lX11 -lXrandr

CC= gcc

LVERS= $(shell . ../lib/shlib_version; echo $$major.$$minor)

all: symlink scrotwm libswmhack

symlink:
	ln -sf ../scrotwm.c ../lib/swm_hack.c .

scrotwm: scrotwm.o linux.o
	$(CC) $(LDFLAGS) -o $@ $+ $(LDADD)

%.so: %.c
	$(CC) $(CFLAGS) -c -fpic -DPIC $+ -o $@

libswmhack: swm_hack.so
	$(CC) -shared -fpic -o libswmhack.so.$(LVERS) `lorder swm_hack.so|tsort`

clean:
	rm -f scrotwm *.o *.so libswmhack.so.*
