# $scrotwm$
.include <bsd.xconf.mk>

PROG=scrotwm
NOMAN=
#MAN=scrotwm.1

CPPFLAGS+= -I${X11BASE}/include
LDADD+=-L${X11BASE}/lib -lX11

MANDIR= ${X11BASE}/man/cat

obj: _xenocara_obj

.include <bsd.prog.mk>
.include <bsd.xorg.mk>
