.include <bsd.xconf.mk>

PREFIX?=/usr/local

LIB=    swmhack
NOMAN=	yes
SRCS=	swm_hack.c
LIBDIR= ${X11BASE}/lib

DEBUGLIBS= no
NOPROFILE= yes

CFLAGS+=-Wall -Wno-uninitialized -g -fPIC
CFLAGS+= -I${X11BASE}/include

install:
	${INSTALL} ${INSTALL_COPY} -o ${LIBOWN} -g ${LIBGRP} -m ${LIBMODE} \
		lib${LIB}.so.${SHLIB_MAJOR}.${SHLIB_MINOR} \
		${PREFIX}/lib/

.include <bsd.lib.mk>
.include <bsd.xorg.mk>
