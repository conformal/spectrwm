#!/bin/sh

CURDIR=$(dirname $0)
if [ -d "$CURDIR/.git" ]; then
	cd "$CURDIR"
	echo $(git describe --abbrev=0 --tags)-$(git rev-parse HEAD | tail -c 9)
fi
