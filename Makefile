.include <bsd.xconf.mk>

PREFIX?=/usr/local

BINDIR=${PREFIX}/bin
SUBDIR= lib

PROG=spectrwm
MAN=spectrwm.1

CFLAGS+=-std=c99 -Wmissing-prototypes -Wall -Wextra -Wshadow -Wno-uninitialized -g
# Uncomment define below to disallow user settable clock format string
#CFLAGS+=-DSWM_DENY_CLOCK_FORMAT
CPPFLAGS+= -I${X11BASE}/include -I${X11BASE}/include/freetype2
LDADD+=-lutil -L${X11BASE}/lib -lX11 -lX11-xcb -lxcb -lxcb-util -lxcb-icccm -lxcb-keysyms -lxcb-randr -lxcb-xinput -lxcb-xtest -lXft -lXcursor
BUILDVERSION != sh "${.CURDIR}/buildver.sh"
.if !${BUILDVERSION} == ""
CPPFLAGS+= -DSPECTRWM_BUILDSTR=\"$(BUILDVERSION)\"
.endif

MANDIR= ${PREFIX}/man/man

obj: _xenocara_obj

beforeinstall:
	ln -sf ${PROG} ${BINDIR}/scrotwm

spectrwm.html: spectrwm.1
	mandoc -Thtml ${.CURDIR}/spectrwm.1 > spectrwm.html

.include <bsd.prog.mk>
.include <bsd.xorg.mk>
