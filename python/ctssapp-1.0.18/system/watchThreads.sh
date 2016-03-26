#!/bin/bash
# ------------------------------------------------------------------------
# watchThreads.sh
#
# Periodically print the voluntary and nonvoluntary context switches for 
# the named process. If no command line argument is given, defaults to 
# display.
#
# If the context switches don't change over time, we can consider that
# the threads that aren't changing are hung somehow.
# ------------------------------------------------------------------------

pname=display
[ $# -gt 0 ] && pname="$1"

printf "watching $pname" 

while true; do
    printf "\nprocess pid     voluntary switches     nonvoluntary switches\n"

    for p in $(ps -e | awk '/'$pname'/ {print $1}'); do
        [ -f /proc/$p/status ] || continue
        printf "%-15d " $p
        cat /proc/$p/status | awk '
            /^voluntary_ctxt_switches:/ { vol = $2 }
            /^nonvoluntary_ctxt_switches:/ { nonvol = $2 }
        END {
            printf ("%-22d %d\n",vol,nonvol)
        }'
    done

    sleep 5
done