PREFIX       ?= /usr/local
BINDIR       ?= $(PREFIX)/bin
LIBDIR       ?= $(PREFIX)/lib
DATAROOTDIR  ?= $(PREFIX)/share
MANDIR       ?= $(DATAROOTDIR)/man
XSESSIONSDIR ?= $(DATAROOTDIR)/xsessions

BUILDVERSION    = $(shell sh $(CURDIR)/../buildver.sh)
LIBVERSION      = $(shell .  $(CURDIR)/../lib/shlib_version; echo $$major.$$minor)
LIBMAJORVERSION = $(shell .  $(CURDIR)/../lib/shlib_version; echo $$major)

MAINT_CFLAGS   = -std=c99 -Wmissing-prototypes -Wall -Wextra -Wshadow -Wno-uninitialized -g
MAINT_CPPFLAGS = -D_GNU_SOURCE -I. -I/usr/include/freetype2 -DSWM_LIB=\"$(LIBDIR)/libswmhack.so.$(LIBVERSION)\"
MAINT_LDLIBS   = -lX11 -lX11-xcb -lxcb -lxcb-icccm -lxcb-randr -lxcb-keysyms -lxcb-util -lxcb-xtest -lXft -lXcursor

ifneq ("${BUILDVERSION}", "")
MAINT_CPPFLAGS += -DSPECTRWM_BUILDSTR=\"$(BUILDVERSION)\"
endif

all: spectrwm libswmhack.so.$(LIBVERSION)

spectrwm: spectrwm.o linux.o
	$(CC) $(MAINT_LDFLAGS) $(LDFLAGS) -o $@ $+ $(MAINT_LDLIBS) $(LDLIBS)

spectrwm.o: ../spectrwm.c ../version.h tree.h util.h
	$(CC) $(MAINT_CFLAGS) $(CFLAGS) $(MAINT_CPPFLAGS) $(CPPFLAGS) -c -o $@ $<

linux.o: linux.c util.h
	$(CC) $(MAINT_CFLAGS) $(CFLAGS) $(MAINT_CPPFLAGS) $(CPPFLAGS) -c -o $@ $<

libswmhack.so.$(LIBVERSION): swm_hack.so
	$(CC) $(MAINT_LDFLAGS) $(LDFLAGS) -Wl,-soname,$@ -shared -fpic -o $@ $+ $(MAINT_LDLIBS) $(LDLIBS)

swm_hack.so: ../lib/swm_hack.c
	$(CC) $(MAINT_CFLAGS) $(CFLAGS) $(MAINT_CPPFLAGS) $(CPPFLAGS) -fpic -DPIC -c -o $@ $<

clean:
	rm -f spectrwm *.o libswmhack.so.* *.so

install: all
	install -m 755 -d $(DESTDIR)$(BINDIR)
	install -m 755 -d $(DESTDIR)$(LIBDIR)
	install -m 755 -d $(DESTDIR)$(MANDIR)/man1
	install -m 755 -d $(DESTDIR)$(MANDIR)/es/man1
	install -m 755 -d $(DESTDIR)$(MANDIR)/it/man1
	install -m 755 -d $(DESTDIR)$(MANDIR)/pt/man1
	install -m 755 -d $(DESTDIR)$(MANDIR)/ru/man1
	install -m 755 -d $(DESTDIR)$(XSESSIONSDIR)
	install -m 755 spectrwm                    $(DESTDIR)$(BINDIR)
	ln -sf spectrwm                            $(DESTDIR)$(BINDIR)/scrotwm
	install -m 644 libswmhack.so.$(LIBVERSION) $(DESTDIR)$(LIBDIR)
	ln -sf libswmhack.so.$(LIBVERSION)         $(DESTDIR)$(LIBDIR)/libswmhack.so.$(LIBMAJORVERSION)
	ln -sf libswmhack.so.$(LIBVERSION)         $(DESTDIR)$(LIBDIR)/libswmhack.so
	install -m 644 ../spectrwm.1               $(DESTDIR)$(MANDIR)/man1/spectrwm.1
	install -m 644 ../spectrwm_es.1            $(DESTDIR)$(MANDIR)/es/man1/spectrwm.1
	install -m 644 ../spectrwm_it.1            $(DESTDIR)$(MANDIR)/it/man1/spectrwm.1
	install -m 644 ../spectrwm_pt.1            $(DESTDIR)$(MANDIR)/pt/man1/spectrwm.1
	install -m 644 ../spectrwm_ru.1            $(DESTDIR)$(MANDIR)/ru/man1/spectrwm.1
	install -m 644 spectrwm.desktop            $(DESTDIR)$(XSESSIONSDIR)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/spectrwm
	rm -f $(DESTDIR)$(BINDIR)/scrotwm
	rm -f $(DESTDIR)$(LIBDIR)/libswmhack.so.$(LIBVERSION)
	rm -f $(DESTDIR)$(LIBDIR)/libswmhack.so.$(LIBMAJORVERSION)
	rm -f $(DESTDIR)$(LIBDIR)/libswmhack.so
	rm -f $(DESTDIR)$(MANDIR)/man1/spectrwm.1
	rm -f $(DESTDIR)$(MANDIR)/es/man1/spectrwm.1
	rm -f $(DESTDIR)$(MANDIR)/it/man1/spectrwm.1
	rm -f $(DESTDIR)$(MANDIR)/pt/man1/spectrwm.1
	rm -f $(DESTDIR)$(MANDIR)/ru/man1/spectrwm.1
	rm -f $(DESTDIR)$(XSESSIONSDIR)/spectrwm.desktop

.PHONY: all clean install uninstall
