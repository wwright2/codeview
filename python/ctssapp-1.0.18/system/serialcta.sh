#!/bin/bash

outputtxt="/home/cta/serial"

IFS=":"
searchpath=".:/config:/ctss/bin:/home/luminator:"$PATH
serialpath="/config
serialfile="serial-number"
 
findbin()
{
    for arg ; do
        echo $arg 
        if [ -f "$arg/$serialfile" ] ; then
            serialpath=$arg
            return 0
        fi
    done
}

findbin $searchpath
if [ $? == 0 ] ; then
    cp $serialpath/$serialfile $outputtxt
    chmod 444 $outputtxt
fi
