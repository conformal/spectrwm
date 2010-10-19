# $OpenBSD: Makefile,v 1.23 2010/10/19 08:13:59 espie Exp $

COMMENT=		small tiling window manager

SHARED_LIBS=		swmhack 0.0

DISTNAME=		scrotwm-SCROTWMVERSION
EXTRACT_SUFX=		.tgz
CATEGORIES=		x11

HOMEPAGE=		http://www.scrotwm.org/
MAINTAINER=		Marco Peereboom <marco@peereboom.us>

# BSD
PERMIT_PACKAGE_CDROM=   Yes
PERMIT_PACKAGE_FTP=     Yes
PERMIT_DISTFILES_CDROM= Yes
PERMIT_DISTFILES_FTP=   Yes

MASTER_SITES=	http://opensource.conformal.com/snapshots/scrotwm/

NO_REGRESS=     Yes
WANTLIB=        c X11 Xrandr util

RUN_DEPENDS=	:dmenu->=3.9:x11/dmenu
		
MAKE_FLAGS=	BINDIR="${TRUEPREFIX}/bin" \
		MANDIR="${TRUEPREFIX}/man/cat" \
		SHLIB_MAJOR=${LIBswmhack_VERSION:C/\..*//} \
		SHLIB_MINOR=${LIBswmhack_VERSION:C/.*\.//}
USE_GROFF =	Yes

pre-configure:
	@${SUBST_CMD} ${WRKSRC}/scrotwm.conf ${WRKSRC}/scrotwm.c

post-install:
	${INSTALL_DATA_DIR} ${PREFIX}/share/examples/scrotwm
	${INSTALL_SCRIPT} ${WRKSRC}/baraction.sh ${PREFIX}/share/examples/scrotwm
	${INSTALL_SCRIPT} ${WRKSRC}/initscreen.sh ${PREFIX}/share/examples/scrotwm
	${INSTALL_SCRIPT} ${WRKSRC}/screenshot.sh ${PREFIX}/share/examples/scrotwm
	${INSTALL_DATA} ${WRKSRC}/scrotwm.conf ${PREFIX}/share/examples/scrotwm

.for i in es it pt ru
	${INSTALL_DATA_DIR} ${PREFIX}/man/$i/cat1
	mv ${PREFIX}/man/cat1/scrotwm_$i.0 ${PREFIX}/man/$i/cat1/scrotwm.0
.endfor

.include <bsd.port.mk>
