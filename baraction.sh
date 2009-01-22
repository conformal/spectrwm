#!/bin/sh
while :; do echo "battery" `apm -l` "%"; sleep 1; done
