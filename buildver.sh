#!/bin/sh

CURDIR=$(dirname $0)
if [ -d "$CURDIR/.git" ]; then
	cd "$CURDIR"
	echo $(git rev-parse HEAD)
fi
