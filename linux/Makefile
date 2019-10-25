PREFIX       ?= /usr/local
BINDIR       ?= $(PREFIX)/bin
LIBDIR       ?= $(PREFIX)/lib
SYSCONFDIR   ?= $(PREFIX)/etc
DATAROOTDIR  ?= $(PREFIX)/share
MANDIR       ?= $(DATAROOTDIR)/man
DOCDIR       ?= $(DATAROOTDIR)/doc/spectrwm
XSESSIONSDIR ?= $(DATAROOTDIR)/xsessions

BUILDVERSION    = $(shell sh $(CURDIR)/../buildver.sh)
LIBVERSION      = $(shell .  $(CURDIR)/../lib/shlib_version; echo $$major.$$minor)
LIBMAJORVERSION = $(shell .  $(CURDIR)/../lib/shlib_version; echo $$major)

MAINT_CFLAGS   = -std=c99 -Wmissing-prototypes -Wall -Wextra -Wshadow -Wno-uninitialized -g
MAINT_LDFLAGS  = -Wl,--as-needed
MAINT_CPPFLAGS = -I. -D_GNU_SOURCE -DSWM_LIB=\"$(LIBDIR)/libswmhack.so.$(LIBVERSION)\"

ifneq ("${BUILDVERSION}", "")
MAINT_CPPFLAGS += -DSPECTRWM_BUILDSTR=\"$(BUILDVERSION)\"
endif

BIN_CFLAGS   = -fPIE
BIN_LDFLAGS  = -fPIE -pie
BIN_CPPFLAGS = $(shell pkg-config --cflags x11 x11-xcb xcb-icccm xcb-keysyms xcb-randr xcb-util xcb-xinput xcb-xtest xcursor xft)
BIN_LDLIBS   = $(shell pkg-config --libs   x11 x11-xcb xcb-icccm xcb-keysyms xcb-randr xcb-util xcb-xinput xcb-xtest xcursor xft)
LIB_CFLAGS   = -fPIC
LIB_LDFLAGS  = -fPIC -shared
LIB_CPPFLAGS = $(shell pkg-config --cflags x11)
LIB_LDLIBS   = $(shell pkg-config --libs   x11) -ldl

all: spectrwm libswmhack.so.$(LIBVERSION)

spectrwm: spectrwm.o linux.o
	$(CC) $(MAINT_LDFLAGS) $(BIN_LDFLAGS) $(LDFLAGS) -o $@ $+ $(BIN_LDLIBS) $(LDLIBS)

spectrwm.o: ../spectrwm.c ../version.h tree.h util.h
	$(CC) $(MAINT_CFLAGS) $(BIN_CFLAGS) $(CFLAGS) $(MAINT_CPPFLAGS) $(BIN_CPPFLAGS) $(CPPFLAGS) -c -o $@ $<

linux.o: linux.c util.h
	$(CC) $(MAINT_CFLAGS) $(BIN_CFLAGS) $(CFLAGS) $(MAINT_CPPFLAGS) $(BIN_CPPFLAGS) $(CPPFLAGS) -c -o $@ $<

libswmhack.so.$(LIBVERSION): swm_hack.so
	$(CC) $(MAINT_LDFLAGS) $(LIB_LDFLAGS) $(LDFLAGS) -Wl,-soname,$@ -o $@ $+ $(LIB_LDLIBS) $(LDLIBS)

swm_hack.so: ../lib/swm_hack.c
	$(CC) $(MAINT_CFLAGS) $(LIB_CFLAGS) $(CFLAGS) $(MAINT_CPPFLAGS) $(LIB_CPPFLAGS) $(CPPFLAGS) -c -o $@ $<

clean:
	rm -f spectrwm *.o libswmhack.so.* *.so

install: all
	install -m 755 -d $(DESTDIR)$(BINDIR)
	install -m 755 -d $(DESTDIR)$(LIBDIR)
	install -m 755 -d $(DESTDIR)$(SYSCONFDIR)
	install -m 755 -d $(DESTDIR)$(MANDIR)/man1
	install -m 755 -d $(DESTDIR)$(DOCDIR)
	install -m 755 -d $(DESTDIR)$(XSESSIONSDIR)
	install -m 755 spectrwm                    $(DESTDIR)$(BINDIR)
	ln -sf spectrwm                            $(DESTDIR)$(BINDIR)/scrotwm
	install -m 755 libswmhack.so.$(LIBVERSION) $(DESTDIR)$(LIBDIR)
	ln -sf libswmhack.so.$(LIBVERSION)         $(DESTDIR)$(LIBDIR)/libswmhack.so.$(LIBMAJORVERSION)
	ln -sf libswmhack.so.$(LIBVERSION)         $(DESTDIR)$(LIBDIR)/libswmhack.so
	install -m 644 ../spectrwm.conf            $(DESTDIR)$(SYSCONFDIR)
	install -m 644 ../spectrwm.1               $(DESTDIR)$(MANDIR)/man1
	install -m 644 ../CHANGELOG.md             $(DESTDIR)$(DOCDIR)
	install -m 644 ../LICENSE.md               $(DESTDIR)$(DOCDIR)
	install -m 644 spectrwm.desktop            $(DESTDIR)$(XSESSIONSDIR)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/spectrwm
	rm -f $(DESTDIR)$(BINDIR)/scrotwm
	rm -f $(DESTDIR)$(LIBDIR)/libswmhack.so.$(LIBVERSION)
	rm -f $(DESTDIR)$(LIBDIR)/libswmhack.so.$(LIBMAJORVERSION)
	rm -f $(DESTDIR)$(LIBDIR)/libswmhack.so
	rm -f $(DESTDIR)$(SYSCONFDIR)/spectrwm.conf
	rm -f $(DESTDIR)$(MANDIR)/man1/spectrwm.1
	rm -f $(DESTDIR)$(DOCDIR)/CHANGELOG.md
	rm -f $(DESTDIR)$(DOCDIR)/LICENSE.md
	rm -f $(DESTDIR)$(XSESSIONSDIR)/spectrwm.desktop

.PHONY: all clean install uninstall
