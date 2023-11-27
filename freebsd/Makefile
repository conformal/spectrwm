PORTNAME=	spectrwm
DISTVERSIONPREFIX=	SPECTRWM_
DISTVERSION=	3_5_1
PORTREVISION=	1
CATEGORIES=     x11-wm

MAINTAINER=	pyrus@bsdmail.com
COMMENT=	Small, dynamic tiling window manager for X11
WWW=		https://github.com/conformal/spectrwm

LICENSE=	ISCL

LIB_DEPENDS+= libxcb-util.so:x11/xcb-util            \
			  libxcb-icccm.so:x11/xcb-util-wm        \
			  libxcb-keysyms.so:x11/xcb-util-keysyms \
			  libfontconfig.so:x11-fonts/fontconfig
RUN_DEPENDS+= dmenu:x11/dmenu

USE_GITHUB=	yes
GH_ACCOUNT=	conformal

OPTIONS_DEFINE=	 EXAMPLES
OPTIONS_DEFAUTL= EXAMPLES
EXAMPLES_DESC=   Example configuration files for different langauges

.include	<bsd.port.options.mk>

USES=		compiler:c11 xorg
USE_XORG=	x11 xcb xcursor xft xrandr xt

USE_LDCONFIG=	yes
CONFEXAMPLES=	spectrwm_cz.conf    \
				spectrwm_es.conf    \
				spectrwm_fr.conf    \
				spectrwm_fr_ch.conf \
				spectrwm_se.conf    \
				spectrwm_us.conf

SCRIPTEXAMPLES=	baraction.sh screenshot.sh

CFLAGS+= -std=c99 -Wmissing-prototypes -Wall -Wextra -Wshadow -Werror -Wno-uninitialized   \
		 -I${LOCALBASE}/include           \
		 -I${LOCALBASE}/include/freetype2 \
		 -I${WRKSCR}/freebsd              \
		 -L${LOCALBASE}/lib

LDFLAGS+= -lX11 -lX11-xcb -lxcb -lxcb-icccm -lxcb-keysyms -lxcb-randr \
		  -lxcb-util -lxcb-xinput -lxcb-xtest -lXcursor -lXft         \
		  -lfontconfig -lutil

post-patch:
	@${REINPLACE_CMD} -e 's|/etc/|${PREFIX}/etc/|g' ${WRKSRC}/spectrwm.*

do-build:
	${LN} -sf ${WRKSRC}/linux/pledge.h ${WRKSRC}/pledge.h
	${LN} -sf ${WRKSRC}/linux/queue_compat.h ${WRKSRC}/queue_compat.h

	${CC} ${CFLAGS} ${LDFLAGS} -DSWM_LIB=\"${PREFIX}/lib/libswmhack.so\" \
		-o ${WRKSRC}/${PORTNAME} ${WRKSRC}/${PORTNAME}.c

	${CC} ${CFLAGS} -shared -fpic -DPIC ${LDFLAGS} \
		-o ${WRKSRC}/libswmhack.so ${WRKSRC}/lib/swm_hack.c

pre-install:
	${MKDIR} ${STAGEDIR}${PREFIX}/share/xsessions/
	${MKDIR} ${STAGEDIR}${EXAMPLESDIR}

do-install:
	${INSTALL_PROGRAM} ${WRKSRC}/${PORTNAME} ${STAGEDIR}${PREFIX}/bin
	${INSTALL_LIB}  ${WRKSRC}/libswmhack.so ${STAGEDIR}${PREFIX}/lib
	${INSTALL_MAN}  ${WRKSRC}/spectrwm.1 ${STAGEDIR}${MAN1PREFIX}/man/man1
	${INSTALL_DATA} ${WRKSRC}/spectrwm.conf ${STAGEDIR}${PREFIX}/etc/spectrwm.conf.sample
	${INSTALL_DATA} ${WRKSRC}/linux/spectrwm.desktop ${STAGEDIR}${PREFIX}/share/xsessions/spectrwm.desktop

do-install-EXAMPLES-on:
.for file in ${CONFEXAMPLES}
	${INSTALL_DATA} ${WRKSRC}/${file} ${STAGEDIR}${EXAMPLESDIR}
.endfor

.for file in ${SCRIPTEXAMPLES}
	${INSTALL_SCRIPT} ${WRKSRC}/${file} ${STAGEDIR}${EXAMPLESDIR}
.endfor

.include <bsd.port.mk>
