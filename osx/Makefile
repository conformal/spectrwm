PREFIX?= /usr/local
BINDIR?= $(PREFIX)/bin
LIBDIR?= $(PREFIX)/lib
MANDIR?= $(PREFIX)/share/man

BUILDVERSION    = $(shell sh $(CURDIR)/../buildver.sh)

# To use xquartz, uncomment the following three lines.
INCFLAGS+= -I/opt/X11/include -I/opt/X11/include/freetype2/
LDADD+=  -L/opt/X11/lib -lX11 -lXcursor -lXft

# To use homebrew, uncomment the following three lines.
#INCFLAGS+= -I/opt/X11/include
#LDADD+=  -lX11 -lXcursor -lXft -L/opt/X11/lib
#INCFLAGS+= -I/usr/local/Cellar/freetype/2.5.0.1/include/freetype2

# To use homebrew , comment out the following two lines.
#INCFLAGS+= -I/opt/local/include/freetype2 -I/opt/local/include
#LDADD+=  -L/opt/local/lib -lX11 -lXcursor -lXft

LDADD+=  -lxcb-keysyms -lxcb-util -lxcb-randr -lX11-xcb -lxcb-xtest -lxcb -lxcb-icccm

LVERS= $(shell . ../lib/shlib_version; echo $$major.$$minor)

CFLAGS+= -std=c99 -Wmissing-prototypes -Wall -Wextra -Wshadow -Wno-uninitialized -g
CFLAGS+= -O2 -D_GNU_SOURCE -D__OSX__ -I. -I.. ${INCFLAGS}
CFLAGS+= -DSWM_LIB=\"$(LIBDIR)/libswmhack.so.$(LVERS)\"
ifneq ("${BUILDVERSION}", "")
CFLAGS+= -DSPECTRWM_BUILDSTR=\"$(BUILDVERSION)\"
endif

CC?= cc

all: spectrwm libswmhack.so.$(LVERS)

spectrwm.c:
	ln -sf ../linux/tree.h
	ln -sf ../spectrwm.c
	ln -sf ../version.h
	ln -sf ../linux/pledge.h

swm_hack.c:
	ln -sf ../lib/swm_hack.c

spectrwm: spectrwm.o osx.o
	$(CC) $(LDFLAGS) -o $@ $+ $(LDADD)

%.so: %.c
	$(CC) $(CFLAGS) -c -fpic -DPIC $+ -o $@

libswmhack.so.$(LVERS): swm_hack.so
	$(CC) $(LDFLAGS) -shared -fpic -o libswmhack.so.$(LVERS) swm_hack.so $(LDADD)
# replace above line with this for OSX 10.5
#	$(CC) -shared -bundle -fpic -o libswmhack.so.$(LVERS) $(LDADD) swm_hack.so

install: all
	install -m 755 -d $(DESTDIR)$(BINDIR)
	install -m 755 -d $(DESTDIR)$(LIBDIR)
	install -m 755 -d $(DESTDIR)$(MANDIR)/man1
	install -m 755 spectrwm $(DESTDIR)$(BINDIR)
	install -m 755 libswmhack.so.$(LVERS) $(DESTDIR)$(LIBDIR)
	install -m 644 ../spectrwm.1 $(DESTDIR)$(MANDIR)/man1/spectrwm.1
	ln -sf spectrwm $(DESTDIR)$(BINDIR)/scrotwm
	ln -sf libswmhack.so.0.0 $(DESTDIR)$(LIBDIR)/libswmhack.so.0
	ln -sf libswmhack.so.0.0 $(DESTDIR)$(LIBDIR)/libswmhack.so

clean:
	rm -f spectrwm *.o *.so libswmhack.so.* spectrwm.c swm_hack.c tree.h version.h pledge.h

.PHONY: all install clean
