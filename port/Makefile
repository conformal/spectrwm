# $OpenBSD: Makefile,v 1.10 2009/09/30 00:54:06 sthen Exp $

COMMENT=		small tiling window manager

SHARED_LIBS=		swmhack 0.0

DISTNAME=		scrotwm-SCROTWMVERSION
EXTRACT_SUFX=		.tgz
CATEGORIES=		x11

HOMEPAGE=		http://www.scrotwm.org/
MAINTAINER=		Pierre-Yves Ritschard <pyr@spootnik.org>

# BSD
PERMIT_PACKAGE_CDROM=   Yes
PERMIT_PACKAGE_FTP=     Yes
PERMIT_DISTFILES_CDROM= Yes
PERMIT_DISTFILES_FTP=   Yes

MASTER_SITES=	http://www.peereboom.us/scrotwm/snapshot/

NO_REGRESS=     Yes
WANTLIB=        c X11 Xrandr util

RUN_DEPENDS=	:dmenu->=3.9:x11/dmenu
		
MAKE_FLAGS=	BINDIR="${TRUEPREFIX}/bin" \
		MANDIR="${TRUEPREFIX}/man/cat" \
		SHLIB_MAJOR=${LIBswmhack_VERSION:C/\..*//} \
		SHLIB_MINOR=${LIBswmhack_VERSION:C/.*\.//}

pre-configure:
	@${SUBST_CMD} ${WRKSRC}/scrotwm.conf ${WRKSRC}/scrotwm.c

post-install:
	${INSTALL_DATA_DIR} ${PREFIX}/share/scrotwm
	${INSTALL_DATA_DIR} ${PREFIX}/share/examples/scrotwm
	${INSTALL_SCRIPT} ${WRKSRC}/baraction.sh ${PREFIX}/share/scrotwm
	${INSTALL_SCRIPT} ${WRKSRC}/initscreen.sh ${PREFIX}/share/scrotwm
	${INSTALL_SCRIPT} ${WRKSRC}/screenshot.sh ${PREFIX}/share/scrotwm
	${INSTALL_DATA} ${WRKSRC}/scrotwm.conf ${PREFIX}/share/examples/scrotwm

.include <bsd.port.mk>
