#!/bin/sh

print_date() {
	# The date is printed to the status bar by default.
	# To print the date through this script, set clock_enabled to 0
	# in scrotwm.conf.  Uncomment "print_date" below.
	FORMAT="%a %b %d %R %Z %Y"
	DATE=`date "+${FORMAT}"`
	echo -n "${DATE}     "
}

_print_cpu() {
	typeset -R4 _1=${1} _2=${2} _3=${3} _4=${4} _5=${5}
	echo -n "CPU:${_1}% User${_2}% Nice${_3}% Sys${_4}% Int${_5}% Idle     "
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

print_apm() {
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

while :; do
	# instead of sleeping, use iostat as the update timer.
	# cache the output of apm(8), no need to call that every second.
	/usr/sbin/iostat -C -c 3600 |&	# wish infinity was an option
	APM_DATA=""
	I=0
	while read -p; do
		if [ $(( ${I} % 1 )) -eq 0 ]; then
			APM_DATA=`/usr/sbin/apm -alb`
		fi
		if [ $I -gt 2 ]; then
			# print_date
			print_cpu $REPLY
			print_apm $APM_DATA
			echo ""
		fi
		I=$(( ${I} + 1 ));
	done
done
