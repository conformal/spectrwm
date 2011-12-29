BINDIR?= $(PREFIX)/bin
LIBDIR?= $(PREFIX)/lib
MANDIR?= $(PREFIX)/share/man

LVERS!=	awk -F = '/major/ { printf( "%s.", $$2 ) } /minor/ { printf( "%s", $$2 ) }' ${.CURDIR}/../lib/shlib_version
BUILDVERSION!= sh "${.CURDIR}/../buildver.sh"
.if !${BUILDVERSION} == ""
CFLAGS+= -DSCROTWM_BUILDSTR=\"$(BUILDVERSION)\"
.endif

CFLAGS+= -Wall -Wno-uninitialized -I. -I${X11BASE}/include
CFLAGS+= -DSWM_LIB=\"$(LIBDIR)/libswmhack.so.$(LVERS)\"
LDADD+= -lutil -L${X11BASE}/lib -lX11 -lXrandr -lXtst


all: scrotwm libswmhack.so.$(LVERS)

scrotwm.c:
	ln -sf ../scrotwm.c
	ln -sf ../version.h

swm_hack.c:
	ln -sf ../lib/swm_hack.c

scrotwm: scrotwm.o
	$(CC) $(LDFLAGS) $(LDADD) -o ${.TARGET} ${.ALLSRC}

swm_hack.so: swm_hack.c
	$(CC) $(CFLAGS) -c -shared -fpic -DPIC -o ${.TARGET} ${.ALLSRC}

libswmhack.so.$(LVERS): swm_hack.so
	$(CC) $(LDFLAGS) -shared -fpic -o ${.TARGET} ${.ALLSRC}

install: all
	install -m 755 -d $(DESTDIR)$(BINDIR)
	install -m 755 -d $(DESTDIR)$(LIBDIR)
	install -m 755 -d $(DESTDIR)$(MANDIR)/man1
	install -m 755 scrotwm $(DESTDIR)$(BINDIR)
	install -m 755 libswmhack.so.$(LVERS) $(DESTDIR)$(LIBDIR)
	install -m 644 ../scrotwm.1 $(DESTDIR)$(MANDIR)/man1/scrotwm.1
	install -m 644 ../scrotwm_es.1 $(DESTDIR)$(MANDIR)/man1/scrotwm_es.1
	install -m 644 ../scrotwm_it.1 $(DESTDIR)$(MANDIR)/man1/scrotwm_it.1
	install -m 644 ../scrotwm_pt.1 $(DESTDIR)$(MANDIR)/man1/scrotwm_pt.1
	install -m 644 ../scrotwm_ru.1 $(DESTDIR)$(MANDIR)/man1/scrotwm_ru.1

clean:
	rm -f scrotwm *.o *.so libswmhack.so.* scrotwm.c swm_hack.c version.h

.include <bsd.port.mk>
