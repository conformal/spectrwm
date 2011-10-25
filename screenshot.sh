#!/bin/sh
#

screenshot() {
	case $1 in
	full)
		scrot -m
		;;
	window)
		sleep 1
		scrot -s
		;;
	*)
		;;
	esac;
}

screenshot $1
