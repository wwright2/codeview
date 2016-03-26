#!/bin/bash
# ----------------------------------------------------------------------------
# dispTest.sh 
#
# This reads lines from msgs.txt (or whatever is given on the command line)
# and sends them to the display process. Each line in the file must be a
# message formatted as it would come from busapi.py. 
#
# The first byte of each message is a binary message id, it won't work if it's
# an ascii number.
#
# The file is read in a loop, so messages will recycle.
# ----------------------------------------------------------------------------


INPUT=msgs.txt

# ----------------------------------------------------------------------------
#
log ()
{
    logger -t dispTest.sh "$@"
    echo "$@"
}

# ----------------------------------------------------------------------------
#
die ()
{
    log "$@"
    exit 1
}


# ----------------------------------------------------------------------------
# start here
#

[ $# -gt 0 ] && INPUT="$1"

exec 7>/dev/udp/localhost/15545

while true; do
    while read line; do
        l=$(printf "$line")
        echo "$l" >&7
        sleep 15
    done < $INPUT
done
