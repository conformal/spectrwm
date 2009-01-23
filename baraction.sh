#!/bin/sh

print_apm() {
	BAT_STATUS=$1
	BAT_LEVEL=$2
	AC_STATUS=$3

	if [ $AC_STATUS -ne 255 -o $BAT_STATUS -lt 4 ]; then
		if [ $AC_STATUS -eq 0 ]; then
			echo "on battery (${BAT_LEVEL}%)"
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
				echo $FULL
			fi
		fi
	fi
}

while :; do
	print_apm `/usr/sbin/apm -alb`
	sleep 59
done
