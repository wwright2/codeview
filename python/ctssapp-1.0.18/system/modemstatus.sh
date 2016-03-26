#!/bin/bash
CTSS="/ctss/bin"
ctssdir=/ctss
systemLogFile="/var/log/messages"

pushd $CTSS > /dev/null

modemcmds='AT!RSSI? AT+CSQ? AT+CAD? AT+CBIP? AT!SUFWDTCSTATS AT!SUFWDCRCS'

# ---------- Include
source $ctssdir/bin/modemdev.sh

modemdev > modemDev
  
echo $modemDev 
echo $modemcmds

# if modemDev not null, then run Serial Command to port. Filter out the \r for XML status.xml file.
#  serialcmd will accept multiple commands at one time.
if [[ -n "$modemDev" ]] ; then
   $CTSS/serialcmd.py -p $modemDev -b 460800 -c "$modemcmds" | sed -e 's/\r//g'  2>&1
else
   echo "No modem Device found, searched for $devicelist" 
fi


popd > /dev/null


