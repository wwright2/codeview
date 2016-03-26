#!/bin/bash


if [[ -z "$1" ]] ; then
	exit 1
fi

tstr="$1"

tz=$(cat /etc/TZ)
if [ "$tz" != "CST6CTD" ]; then
    echo CST6CTD > /etc/TZ
fi

TZ=CST6CDT date -s "$1"
hwclock -u -w


