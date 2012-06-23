CFLAGS+= -O2 -Wall -g -D_GNU_SOURCE -D__OSX__ -I. -I..
CFLAGS+= -DSWM_LIB=\"$(LIBDIR)/libswmhack.so.$(LVERS)\"
LDADD+=  -L/usr/X11/lib/ -lX11 -lXrandr -lXtst

PREFIX?= /usr/local
BINDIR?= $(PREFIX)/bin
LIBDIR?= $(PREFIX)/lib
MANDIR?= $(PREFIX)/share/man

CC= gcc

LVERS= $(shell . ../lib/shlib_version; echo $$major.$$minor)

all: spectrwm libswmhack.so.$(LVERS)

spectrwm.c:
	ln -sf ../spectrwm.c

swm_hack.c:
	ln -sf ../lib/swm_hack.c

spectrwm: spectrwm.o osx.o
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
	install -m 755 spectrwm $(DESTDIR)$(BINDIR)
	install -m 755 libswmhack.so.$(LVERS) $(DESTDIR)$(LIBDIR)
	install -m 644 ../spectrwm.1 $(DESTDIR)$(MANDIR)/man1/spectrwm.1
	install -m 644 ../spectrwm_es.1 $(DESTDIR)$(MANDIR)/man1/spectrwm_es.1
	install -m 644 ../spectrwm_it.1 $(DESTDIR)$(MANDIR)/man1/spectrwm_it.1
	install -m 644 ../spectrwm_pt.1 $(DESTDIR)$(MANDIR)/man1/spectrwm_pt.1
	install -m 644 ../spectrwm_ru.1 $(DESTDIR)$(MANDIR)/man1/spectrwm_ru.1
	ln -sf $(DESTDIR)$(BINDIR)/spectrwm $(DESTDIR)$(BINDIR)/scrotwm

clean:
	rm -f spectrwm *.o *.so libswmhack.so.* spectrwm.c swm_hack.c

.PHONY: all install clean
