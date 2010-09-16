# $scrotwm$
.include <bsd.xconf.mk>

PREFIX?=/usr/local

BINDIR=${PREFIX}/bin
SUBDIR= lib

PROG=scrotwm
MAN=scrotwm.1 scrotwm_es.1 scrotwm_it.1 scrotwm_pt.1 scrotwm_ru.1

CFLAGS+=-std=c89 -Wall -Wno-uninitialized -ggdb3
# Uncomment define below to disallow user settable clock format string
#CFLAGS+=-DSWM_DENY_CLOCK_FORMAT
CPPFLAGS+= -I${X11BASE}/include
LDADD+=-lutil -L${X11BASE}/lib -lX11 -lXrandr

MANDIR= ${PREFIX}/man/cat

scrotwm_ru.cat1: scrotwm_ru.1
	 nroff -mandoc ${.CURDIR}/scrotwm_ru.1 > ${.TARGET}

obj: _xenocara_obj

.include <bsd.prog.mk>
.include <bsd.xorg.mk>
