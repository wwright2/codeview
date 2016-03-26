#!/bin/bash
#
# compile py files and remove leaving only busapi.py
#

CTSS="/ctss/bin"
ctssdir="/ctss"
systemLogFile="/var/log/messages"


uname -a > /home/cta/osversion.txt

#------- >>> From here we only run once.

# If this file does not exists exit, Run only if tracker.py file...
#   i.e.  .py file means we recieved an update.
if [[ ! -f "$CTSS/tracker.py" ]] ; then
    exit 0
fi


# Compile PYTHON and remove extra .py files.    
./compile.py

exceptions="busapi.pyc generatorBusapi.pyc compile.pyc serialcmd.pyc"
rm $exceptions

# remove py files.
for i in `ls *.pyc` ; do 
    x=`echo $i |cut -d'.' -f1`; 
    echo $x; 
    if [[ -f $x".py" ]] ; then
        rm $x".py"; 
    fi
done


