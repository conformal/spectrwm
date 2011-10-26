#!/bin/sh

CURDIR=$(dirname $0)
if [ -d "$CURDIR/.git" ]; then
	cd "$CURDIR"
	git describe --tags | tr -d '\n'
fi
