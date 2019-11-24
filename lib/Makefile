.include <bsd.xconf.mk>

PREFIX?=/usr/local

LIB=    swmhack
NOMAN=	yes
SRCS=	swm_hack.c
LIBDIR= ${X11BASE}/lib

CC?= cc

DEBUGLIBS= no
NOPROFILE= yes

CFLAGS+= -std=c99 -Wmissing-prototypes -Wall -Wextra -Wshadow -Wno-uninitialized -g
CFLAGS+= -fPIC
CFLAGS+= -I${X11BASE}/include
LDADD+= -L${X11BASE}/lib -lX11

install:
	${INSTALL} ${INSTALL_COPY} -o ${LIBOWN} -g ${LIBGRP} -m ${LIBMODE} \
		lib${LIB}.so.${SHLIB_MAJOR}.${SHLIB_MINOR} \
		${PREFIX}/lib/

.include <bsd.lib.mk>
.include <bsd.xorg.mk>
