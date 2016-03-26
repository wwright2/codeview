

#  Common subroutine.
#
# called from pppmonitor, modemstatus, runonce.sh

devicelist='/dev/ttyACM0 /dev/ttyUSB0'
modemDev=""

modemOn=1       # non-zero Power-on
modemOff=0      # zero  Power-off



modemdev()
{
	# ------------------------------------------
	# set default, then try to detect presence.
	# accept the first one found as the modem device.
	
	for dev in $devicelist ; do 
	   if [[ -c "$dev" ]] ; then
		  modemDev=$dev
		  break
	   fi
	done
	
	echo $modemDev
}


waitForDevice ()
{
    local -i acmCnt=0
    logger  " Wait For PNP to bring up ACM0"
    
    # This While loop was written to minimize wait time
    #   ...rather than guessing when ACM0 is available,poll every 2 sec
    #
    while true; do
        # wait MAX period for ACM to show up
        acmCnt="$acmCnt+1"
        if [[ ! `ls $modemDev`  ]] ; then
            echo "device not found, $?"
            sleep 2
         else
            echo "device found"
            break               # break from while.
         fi

        # Max Wait 5*2 = 10sec
        if (( $acmCnt > 5 )) ; then
            logger "Error waiting for $modemDev to become available"
            break
        fi
    done
    logger  " Exit Wait For PNP to bring up ACM0"
}

# updateDiagnostics()
# $1 = count integer
# $2 = file to update.
updateDiagnostics()
{
    local -i cnt=$1
    local -i curcnt=$1
    local file=$2
    
    # Read existing file. Add cnt. Update file.
    curcnt=`cat $file`
    cnt="$cnt+$curcnt"
    echo "$cnt" > $file
}

resetModem ()
{
    # So, a GPIO has been added to reset the modem.
    # We will log the status of $device and the AT response to AT!status
    #

    logger "Reset the Modem"
    echo $
    setgpio $modemOff
    sleep 2
    setgpio $modemOn
#    echo 0 > $pppCntFile
    
    # This While loop was written to minimize wait time
    #   ...rather than guessing when ACM0 is available,poll every 2 sec
    #
    waitForDevice
    modemResetCnt="$modemResetCnt+1"
    updateDiagnostics $modemResetCnt $modemResetDiagnostic
    
    echo "$modemResetCnt" > $modemResetFile
    logger "Modem Reset count = $modemResetCnt" 

    #----------------------------
    # Log some info about Modem #
    # ? ..the device exists 
    # ? ..modem responds
    ls -l $modemDev >> $systemLogFile 2>&1
    if [[ -f "$ctssdir/bin/serialcmd.py" ]] ; then
        # try to send an AT command.
        $ctssdir/bin/serialcmd.py -p $modemDev -b 460800 -c 'at!status' >> $systemLogFile 2>&1
        sleep 1
    fi
}

