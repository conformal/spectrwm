.include <bsd.xconf.mk>

PREFIX?=/usr/local

BINDIR=${PREFIX}/bin
SUBDIR= lib

PROG=scrotwm
#MAN=scrotwm_pt.1 scrotwm_ru.1
MAN=scrotwm.1 scrotwm_es.1 scrotwm_it.1

CFLAGS+=-std=c89 -Wall -Wno-uninitialized -ggdb3
# Uncomment define below to disallow user settable clock format string
#CFLAGS+=-DSWM_DENY_CLOCK_FORMAT
CPPFLAGS+= -I${X11BASE}/include
LDADD+=-lutil -L${X11BASE}/lib -lX11 -lXrandr -lXtst
BUILDVERSION != sh "${.CURDIR}/buildver.sh"
.if !${BUILDVERSION} == ""
CPPFLAGS+= -DSCROTWM_BUILDSTR=\"$(BUILDVERSION)\"
.endif

MANDIR= ${PREFIX}/man/man

#scrotwm_ru.cat1: scrotwm_ru.1
#	 nroff -mandoc ${.CURDIR}/scrotwm_ru.1 > ${.TARGET}

obj: _xenocara_obj

# clang targets
.if ${.TARGETS:M*analyze*}
CC=clang
CXX=clang++
CPP=clang -E
CFLAGS+=--analyze
.elif ${.TARGETS:M*clang*}
CC=clang
CXX=clang++
CPP=clang -E
.endif

analyze: all
clang: all
.include <bsd.prog.mk>
.include <bsd.xorg.mk>
