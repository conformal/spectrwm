.sinclude <bsd.pkg.pre.mk>

PREFIX?= /usr/pkg
LOCALBASE?= /usr/pkg

SWM_BINDIR?= ${DESTDIR}$(PREFIX)/bin
SWM_LIBDIR?= ${DESTDIR}$(PREFIX)/lib
SWM_MANDIR?= ${DESTDIR}$(PREFIX)/${PKGMANDIR}

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
	$(BSD_INSTALL_PROGRAM_DIR) $(SWM_BINDIR)
	$(BSD_INSTALL_LIB_DIR) $(SWM_LIBDIR)
	$(BSD_INSTALL_MAN_DIR) $(SWM_MANDIR)/man1
	$(BSD_INSTALL_PROGRAM) spectrwm $(SWM_BINDIR)
	$(BSD_INSTALL_LIB) libswmhack.so.$(LVERS) $(SWM_LIBDIR)
	$(BSD_INSTALL_MAN) ../spectrwm.1 $(SWM_MANDIR)/man1/spectrwm.1
	ln -sf spectrwm $(SWM_BINDIR)/scrotwm

clean:
	rm -f spectrwm *.o *.so libswmhack.so.* spectrwm.c swm_hack.c version.h

.PHONY:	all install clean

.sinclude <bsd.pkg.post.mk>
