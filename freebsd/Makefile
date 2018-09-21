.sinclude <bsd.port.pre.mk>

PREFIX?= /usr/local
LOCALBASE?= /usr/local

SWM_BINDIR?= $(PREFIX)/bin
SWM_LIBDIR?= $(PREFIX)/lib
SWM_MANDIR?= $(PREFIX)/man

CC?= cc

LVERS!=	awk -F = '/major/ { printf( "%s.", $$2 ) } /minor/ { printf( "%s", $$2 ) }' ${.CURDIR}/../lib/shlib_version
BUILDVERSION!= sh "${.CURDIR}/../buildver.sh"
.if !${BUILDVERSION} == ""
CFLAGS+= -DSPECTRWM_BUILDSTR=\"$(BUILDVERSION)\"
.endif

CFLAGS+= -std=c99 -Wmissing-prototypes -Wall -Wextra -Wshadow -Wno-uninitialized -g
CFLAGS+= -I. -I${LOCALBASE}/include -I${LOCALBASE}/include/freetype2
CFLAGS+= -DSWM_LIB=\"$(SWM_LIBDIR)/libswmhack.so.$(LVERS)\"
LDADD+= -lutil -L${LOCALBASE}/lib -lX11 -lX11-xcb -lxcb \
	-lxcb-icccm -lxcb-keysyms -lxcb-randr -lxcb-util -lxcb-xinput -lxcb-xtest -lXft -lXcursor


all: spectrwm libswmhack.so.$(LVERS)

spectrwm.c:
	ln -sf ../spectrwm.c
	ln -sf ../version.h
	ln -sf ../linux/pledge.h

swm_hack.c:
	ln -sf ../lib/swm_hack.c

spectrwm: spectrwm.o
	$(CC) $(LDFLAGS) $(LDADD) -o ${.TARGET} ${.ALLSRC}

swm_hack.so: swm_hack.c
	$(CC) $(CFLAGS) -c -shared -fpic -DPIC -o ${.TARGET} ${.ALLSRC}

libswmhack.so.$(LVERS): swm_hack.so
	$(CC) $(LDFLAGS) -shared -fpic -o ${.TARGET} ${.ALLSRC}

install: all
	install -m 755 -d $(SWM_BINDIR)
	install -m 755 -d $(SWM_LIBDIR)
	install -m 755 -d $(SWM_MANDIR)/man1
	install -m 755 spectrwm $(SWM_BINDIR)
	install -m 755 libswmhack.so.$(LVERS) $(SWM_LIBDIR)
	install -m 644 ../spectrwm.1 $(SWM_MANDIR)/man1/spectrwm.1
	ln -sf spectrwm $(SWM_BINDIR)/scrotwm

clean:
	rm -f spectrwm *.o *.so libswmhack.so.* spectrwm.c swm_hack.c version.h pledge.h

.PHONY:	all install clean

.sinclude <bsd.port.post.mk>
