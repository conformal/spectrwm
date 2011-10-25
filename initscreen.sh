#!/bin/sh
#
# Example xrandr multiscreen init

xrandr --output LVDS --auto
xrandr --output VGA --auto --right-of LVDS
