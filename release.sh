#!/bin/ksh
#
# $scrotwm$

PREFIX=scrotwm-
DIRS="lib linux"
FILES="Makefile baraction.sh initscreen.sh screenshot.sh scrotwm.1 scrotwm_es.1 scrotwm_it.1 scrotwm.c scrotwm.conf linux/Makefile linux/linux.c linux/util.h lib/Makefile lib/shlib_version lib/swm_hack.c"

if [ -z "$1" ]; then
	echo "usage: release.sh <version>"
	exit 1
fi

if [ -d "$PREFIX$1" ]; then
	echo "$PREFIX$1 already exists"
	exit 1
fi

if [ -d "$PREFIX$1-port" ]; then
	echo "$PREFIX$1 already exists"
	exit 1
fi

TARGET="$PREFIX$1"
mkdir $TARGET

for i in $DIRS; do
	mkdir "$TARGET/$i"
done

for i in $FILES; do
	cp $i "$TARGET/$i"
done

tar zcf $TARGET.tgz $TARGET

# make port
sudo rm -rf ports
sudo cvs -d /cvs co ports/x11/scrotwm
PORT="$PREFIX$1-port"
mkdir $PORT

# Makefile
cat port/Makefile | sed "s/SCROTWMVERSION/$1/g" > $PORT/Makefile

# distinfo
cksum -b -a md5 $TARGET.tgz > $PORT/distinfo
cksum -b -a rmd160 $TARGET.tgz >> $PORT/distinfo
cksum -b -a sha1 $TARGET.tgz >> $PORT/distinfo
cksum -b -a sha256 $TARGET.tgz >> $PORT/distinfo
wc -c $TARGET.tgz 2>/dev/null | awk '{print "SIZE (" $2 ") = " $1}' >> $PORT/distinfo

# pkg
mkdir $PORT/pkg
cp port/pkg/DESCR $PORT/pkg/
cp port/pkg/PFRAG.shared $PORT/pkg/
cp port/pkg/PLIST $PORT/pkg/

# patches
mkdir $PORT/patches
cp port/patches/patch-scrotwm_c $PORT/patches/
cp port/patches/patch-scrotwm_conf $PORT/patches/

# make diff
diff -ruNp -x CVS ports/x11/scrotwm/ $PORT > $TARGET.diff
