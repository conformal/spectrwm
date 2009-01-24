#!/bin/sh

print_cpu() {
	echo -n "CPU: ${7}% User  ${8}% Nice  ${9}% Sys  ${10}% Int  ${11}% Idle     "
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
	# you probably want to reduce the sleep below if you enable this
	#print_cpu `/usr/sbin/iostat -C`
	print_apm `/usr/sbin/apm -alb`
	echo ""
	sleep 1
done
