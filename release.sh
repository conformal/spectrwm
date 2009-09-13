#!/bin/ksh
#
# $scrotwm$

PREFIX=scrotwm-
DIRS="lib linux"
FILES="Makefile baraction.sh initscreen.sh screenshot.sh scrotwm.1 scrotwm.c scrotwm.conf linux/Makefile linux/linux.c linux/swm-linux.diff linux/util.h lib/Makefile lib/shlib_version lib/swm_hack.c"

if [ -z "$1" ]; then
	echo "usage: release.sh <version>"
	exit 1
fi

if [ -d "$PREFIX$1" ]; then
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
