#!/bin/bash
# -----------------------------------------------------------------------
# monitor.sh - This runs a single program instance, and restarts it if
# it fails. Logging, et. al. is also done as needed.
#
# I/O redirection must be done by whatever calls this.
# -----------------------------------------------------------------------

source /ctss/bin/ldlibrary.sh

log ()
{
    echo "$@" 2>&1
    logger -t monitor.sh "$@"
}

die ()
{
    log "$@"
    exit 1
}

endItAll ()
{
    killall $PROG
    exit 0
}

[ $# -lt 1 ] && die "No arguments given to $0"

ARGS=''
PROG=$1
shift
[ $# -ge 1 ] && ARGS="$@"

trap ''  SIGHUP
trap 'endItAll' SIGTERM SIGINT

count=0
maxRespawn=10
respawnTimeout=5
lastTime=$SECONDS

checkRecycleTime ()
{
    interval=$(($SECONDS - $lastTime))
    if [ $interval -lt $respawnTimeout ]; then
        if [ $count -gt $maxRespawn ]; then 
            log "$PROG respawning too rapidly, pausing before next respawn"
            count=0
            sleep 10
            lastTime=$SECONDS
        fi
        count=$((count + 1))
    else
        lastTime=$SECONDS
        count=0
    fi
}


while true; do
    log "Running $PROG $ARGS"
    $PROG "$ARGS"
    count=$((count + 1))
    checkRecycleTime
    log "$PROG returned $? , pausing before restart"
    sleep 3
done

die "Unexpected termination of $0"
