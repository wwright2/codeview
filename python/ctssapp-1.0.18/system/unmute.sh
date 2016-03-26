#!/bin/bash

# set volume
# else unmute using the volume file "volume.settings"

volumeFile="/ctss/cfg/volume.settings"
ctaVolumeFile="/home/cta/volume.txt"
Master="Master"

# create file. 0..33 = 0..99%
#echo "# volume 0..33 for 0..99%"
if [[ -z "$1" ]] ; then
    if [[ ! -f $volumeFile ]]; then
        echo "$Master:27" > $volumeFile
    fi
else
    echo "$Master:$1" > $volumeFile
fi
chown luminator:luminator $volumeFile

# get values, Master, Master Mono, PCM
MasterVol=`cat $volumeFile | grep "$Master" | cut -d':' -f2`    

# Sound Card ? or driver not loaded or correct
if [[ ! -d "/proc/asound/card0" ]] ; then
    echo "No Sound Card Found"
    sleep 2
    exit 1
fi

if [[ $MasterVol == 0 ]]; then
    # MUTE
    amixer -c 0 sset 'Master',0 0,0 mute > /dev/null
    amixer -c 0 sset 'Master Mono',0 0,0 mute > /dev/null
    amixer -c 0 sset 'PCM',0 0,0 mute > /dev/null
    
elif [[ $MasterVol < 34 ]]; then
    vol=$((MasterVol * 3))
    amixer -c 0 sset 'Master',0 "$vol%","$vol%" unmute > /dev/null
    amixer -c 0 sset 'Master Mono',0 "$vol%","$vol%" unmute > /dev/null
    amixer -c 0 sset 'PCM',0 "$vol%","$vol%" unmute > /dev/null
fi


# Dependent code Volume adjust formula tracker.py
let "v=$MasterVol"
let "v=($v-9)/2"
echo $v
echo "$v" > $ctaVolumeFile
chown luminator:luminator $ctaVolumeFile

exit 0