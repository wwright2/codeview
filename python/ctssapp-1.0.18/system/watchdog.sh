#!/bin/bash
#
# tickle the watchdog timer every few Seconds.
#

CTSS="/ctss/bin"

while true ; do 
    $CTSS/watchdog -e120
    sleep 100
done

