# $scrotwm$ 

CFLAGS+= -Wall -Wno-uninitialised -ggdb -D_GNU_SOURCE -I.
LDADD+= -lX11 -lXrandr

CC= gcc

all: scrotwm

scrotwm: scrotwm.o linux.o
	$(CC) $(LDFLAGS) -o $@ $+ $(LDADD)

clean:
	rm -f *.o
