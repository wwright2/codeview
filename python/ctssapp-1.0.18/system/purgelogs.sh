#!/bin/bash

MSGFILE="messages"

usage ()
{
  echo "$0 keepnum"
  echo " ...will remove all but the last keepnum message files."
}

# SET number of log files to keep
keepnum=$1
if [[ -z "$keepnum" ]] ; then
   keepnum=5
fi

pushd /home/cta

x=`ls -rt $MSGFILE* | tail -n $keepnum`
for i in `ls messages*` ; do
    flag="true"
    for j in $x ; do
       if [[ $i == $j ]] ; then
           flag="false"
       fi
    done
    if [[ "$flag" == "true" ]] ; then
       log "purge old log $i"
       rm $i
    fi
done
popd
exit 0


