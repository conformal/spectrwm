#!/bin/sh
# Example Bar Action Script for OpenBSD-current.
#

print_date() {
	# The date is printed to the status bar by default.
	# To print the date through this script, set clock_enabled to 0
	# in spectrwm.conf.  Uncomment "print_date" below.
	FORMAT="%a %b %d %R %Z %Y"
	DATE=`date "+${FORMAT}"`
	echo -n "${DATE}     "
}

print_mem() {
	MEM=`/usr/bin/top | grep Free: | cut -d " " -f6`
	echo -n "Free mem: $MEM  "
}

_print_cpu() {
	printf "CPU: %3d%% User %3d%% Nice %3d%% Sys %3d%% Spin %3d%% Int %3d%% Idle  " $1 $2 $3 $4 $5 $6
}

print_cpu() {
	OUT=""
	# iostat prints each column justified to 3 chars, so if one counter
	# is 100, it jams up agains the preceeding one. sort this out.
	while [ "${1}x" != "x" ]; do
		if [ ${1} -gt 99 ]; then
			OUT="$OUT ${1%100} 100"
		else
			OUT="$OUT ${1}"
		fi
		shift;
	done
	_print_cpu $OUT
}

print_cpuspeed() {
	CPU_SPEED=`/sbin/sysctl hw.cpuspeed | cut -d "=" -f2`
	printf "CPU speed: %4d MHz  " $CPU_SPEED
}

print_bat() {
	BAT_STATUS=$1
	BAT_LEVEL=$2
	AC_STATUS=$3

	if [ $AC_STATUS -ne 255 -o $BAT_STATUS -lt 4 ]; then
		if [ $AC_STATUS -eq 0 ]; then
			echo -n "on battery (${BAT_LEVEL}%)"
		else
			case $AC_STATUS in
			1)
				AC_STRING="on AC: "
				;;
			2)
				AC_STRING="on backup AC: "
				;;
			*)
				AC_STRING=""
				;;
			esac;
			case $BAT_STATUS in
			4)
				BAT_STRING="(no battery)"
				;;
			[0-3])
		 		BAT_STRING="(battery ${BAT_LEVEL}%)"
				;;
			*)
				BAT_STRING="(battery unknown)"
				;;
			esac;

			FULL="${AC_STRING}${BAT_STRING}"
			if [ "$FULL" != "" ]; then
				echo -n "$FULL"
			fi
		fi
	fi
}

# cache the output of apm(8), no need to call that every second.
APM_DATA=""
I=0
while :; do
	IOSTAT_DATA=`/usr/sbin/iostat -C -c 2 | tail -n 1 | grep '[0-9]$'`
	if [ $I -eq 0 ]; then
		APM_DATA=`/usr/sbin/apm -alb`
	fi
	# print_date
	print_mem
	print_cpu $IOSTAT_DATA
	print_cpuspeed
	print_bat $APM_DATA
	echo ""
	I=$(( ( ${I} + 1 ) % 11 ))
	sleep 1
done
