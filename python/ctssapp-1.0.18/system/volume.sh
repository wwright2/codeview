#!/bin/bash

volumeFile="/ctss/cfg/volume.settings"
Master="Master"
MMono="Master Mono"
Pcm="PCM"

if [[ -f $volumeFile ]]; then
    # get values, Master, Master Mono, PCM
    MasterVol=`cat $volumeFile | grep "$Master" | cut -d':' -f2`    
else
    # create file. 0..33 = 0..100%
    echo "# volume 0..33 for 0..99%"
    echo "$Master:27" >> $volumeFile
    chown luminator:luminator $volumeFile
fi

# Sound Card ? or driver not loaded or correct
if [[ ! -d "/proc/asound/card0" ]] ; then
    echo "No Sound Card Found"
    sleep 2
    exit 0
fi

while true ; do
    clear
    printf "Please enter  e(x)it (t)est or (v)olume : "
    read VOLUME
    case "$VOLUME" in
        t|T|"test")
                swift "testing, 1,2,3, testing, 1,2,3"  &
                ;;
        v|V|"volume")
                printf "Enter Volume 0..33:"
                read VOLUME
                if (( $VOLUME < 34 )) ; then
                    vol=$((VOLUME * 3))
                    amixer -c 0 sset 'Master',0 "$vol%","$vol%" unmute
                    amixer -c 0 sset 'Master Mono',0 "$vol%","$vol%" unmute
                    amixer -c 0 sset 'PCM',0 "$vol%","$vol%" unmute
                    #save setting.
                    echo "$Master:$VOLUME" > $volumeFile
		    chown luminator:luminator $volumeFile
                else
                    echo "Error Volume must be numeric between 0 and 33, retry"
                    sleep 2
                fi                
                ;;
        x|q|"exit")
            exit 0
            ;;
    esac
done

exit 0