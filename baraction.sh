#!/bin/sh
while :; do echo "battery" `/usr/sbin/apm -l` "%"; sleep 1; done
