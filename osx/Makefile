CFLAGS+= -O2 -Wall -ggdb -D_GNU_SOURCE -D__OSX__ -I.
CFLAGS+= -DSWM_LIB=\"$(LIBDIR)/libswmhack.so.$(LVERS)\"
LDADD+=  -L/usr/X11/lib/ -lX11 -lXrandr -lXtst

PREFIX?= /usr/local
BINDIR?= $(PREFIX)/bin
LIBDIR?= $(PREFIX)/lib
MANDIR?= $(PREFIX)/share/man

CC= gcc

LVERS= $(shell . ../lib/shlib_version; echo $$major.$$minor)

all: scrotwm libswmhack.so.$(LVERS)

scrotwm.c:
	ln -sf ../scrotwm.c

swm_hack.c:
	ln -sf ../lib/swm_hack.c

scrotwm: scrotwm.o osx.o
	$(CC) $(LDFLAGS) -o $@ $+ $(LDADD)

%.so: %.c
	$(CC) $(CFLAGS) -c -fpic -DPIC $+ -o $@

libswmhack.so.$(LVERS): swm_hack.so
	$(CC) -shared -fpic -o libswmhack.so.$(LVERS) $(LDADD) swm_hack.so
# replace above line with this for OSX 10.5
#	$(CC) -shared -bundle -fpic -o libswmhack.so.$(LVERS) $(LDADD) swm_hack.so

install: all
	install -m 755 -d $(DESTDIR)$(BINDIR)
	install -m 755 -d $(DESTDIR)$(LIBDIR)
	install -m 755 -d $(DESTDIR)$(MANDIR)/man1
	install -m 755 scrotwm $(DESTDIR)$(BINDIR)
	install -m 755 libswmhack.so.$(LVERS) $(DESTDIR)$(LIBDIR)
	install -m 644 ../scrotwm.1 $(DESTDIR)$(MANDIR)/man1/scrotwm.1
	install -m 644 ../scrotwm_es.1 $(DESTDIR)$(MANDIR)/man1/scrotwm_es.1
	install -m 644 ../scrotwm_it.1 $(DESTDIR)$(MANDIR)/man1/scrotwm_it.1
	install -m 644 ../scrotwm_pt.1 $(DESTDIR)$(MANDIR)/man1/scrotwm_pt.1
	install -m 644 ../scrotwm_ru.1 $(DESTDIR)$(MANDIR)/man1/scrotwm_ru.1

clean:
	rm -f scrotwm *.o *.so libswmhack.so.* scrotwm.c swm_hack.c

.PHONY: all install clean
