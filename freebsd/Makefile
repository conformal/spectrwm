.sinclude <bsd.port.pre.mk>

PREFIX?= /usr/local
LOCALBASE?= /usr/local

BINDIR?= $(PREFIX)/bin
LIBDIR?= $(PREFIX)/lib
MANDIR?= $(PREFIX)/man

LVERS!=	awk -F = '/major/ { printf( "%s.", $$2 ) } /minor/ { printf( "%s", $$2 ) }' ${.CURDIR}/../lib/shlib_version
BUILDVERSION!= sh "${.CURDIR}/../buildver.sh"
.if !${BUILDVERSION} == ""
CFLAGS+= -DSPECTRWM_BUILDSTR=\"$(BUILDVERSION)\"
.endif

CFLAGS+= -Wall -Wno-uninitialized -I. -I${LOCALBASE}/include
CFLAGS+= -DSWM_LIB=\"$(LIBDIR)/libswmhack.so.$(LVERS)\"
LDADD+= -lutil -L${LOCALBASE}/lib -lX11 -lXrandr -lXtst


all: spectrwm libswmhack.so.$(LVERS)

spectrwm.c:
	ln -sf ../spectrwm.c
	ln -sf ../version.h

swm_hack.c:
	ln -sf ../lib/swm_hack.c

spectrwm: spectrwm.o
	$(CC) $(LDFLAGS) $(LDADD) -o ${.TARGET} ${.ALLSRC}

swm_hack.so: swm_hack.c
	$(CC) $(CFLAGS) -c -shared -fpic -DPIC -o ${.TARGET} ${.ALLSRC}

libswmhack.so.$(LVERS): swm_hack.so
	$(CC) $(LDFLAGS) -shared -fpic -o ${.TARGET} ${.ALLSRC}

install: all
	install -m 755 -d $(DESTDIR)$(BINDIR)
	install -m 755 -d $(DESTDIR)$(LIBDIR)
	install -m 755 -d $(DESTDIR)$(MANDIR)/man1
	install -m 755 spectrwm $(DESTDIR)$(BINDIR)
	install -m 755 libswmhack.so.$(LVERS) $(DESTDIR)$(LIBDIR)
	install -m 644 ../spectrwm.1 $(DESTDIR)$(MANDIR)/man1/spectrwm.1
	install -m 644 ../spectrwm_es.1 $(DESTDIR)$(MANDIR)/man1/spectrwm_es.1
	install -m 644 ../spectrwm_it.1 $(DESTDIR)$(MANDIR)/man1/spectrwm_it.1
	install -m 644 ../spectrwm_pt.1 $(DESTDIR)$(MANDIR)/man1/spectrwm_pt.1
	install -m 644 ../spectrwm_ru.1 $(DESTDIR)$(MANDIR)/man1/spectrwm_ru.1

clean:
	rm -f spectrwm *.o *.so libswmhack.so.* spectrwm.c swm_hack.c version.h

.sinclude <bsd.port.post.mk>
