#!/bin/bash

# --- File Name(s) ---

systemLogFile="/var/log/messages"

ctssdir="/ctss"
cts="/home/cta"

ignoreTln="$ctssdir/.ignore"

# --- working count(s) ---
wgwPowerFile="$ctssdir/wgwPower.cnt"         # power-cycle count
wgwResetFile="$ctssdir/wgwReset.cnt"         # reset count
wgwTnStatusFile="$ctssdir/wgwTnStatus.cnt"   # telnet status fail count 
wgwRgStatusFile="$ctssdir/wgwRgStatus.cnt"   # reg status fail count 
wgwSStrengthFile="$ctssdir/wgwSStrength.cnt" # signal strength fail count 
wgwPingFile="$ctssdir/wgwPing.cnt"           # ping fail count 
wgwRebootFile="$ctssdir/reboot.cnt"          # system reboot count


# --- total count(s) ---
wgwPowerDiags="$cts/wgwPower.txt"         	# total power-cycled count
wgwResetDiags="$cts/wgwReset.txt"         	# total reset count
wgwTnStatusDiags="$cts/wgwTnStatus.txt"   	# total telnet status fail count
wgwRgStatusDiags="$cts/wgwRgStatus.txt"   	# total reg status fail count
wgwSStrengthDiags="$cts/wgwSStrength.txt" 	# total signal strength fail count
wgwPingDiags="$cts/wgwPing.txt"           	# total ping fail count 
wgwRebootDiags="$cts/wgwReboot.txt"       	# total system reboot count

wgwRestartFile="$ctssdir/ppp-restart.cnt"    # dummy file for compatibility

wgwEsnFile="$cts/esn.txt"		# MEID file

# --- Variables ---

declare    state        # state machine state

declare -i tsts         # telnet session status
declare -i rsts	      # registration status 
declare    imei=""		# IMEI number
declare    meid=""      # MEID number

declare -i ss_1xrtt     # 1xRTT signal strength
declare -i ss_evdo      # EV-DO signal strength 

declare -i rbt_max=3		# reboot count maximum
declare -i pwr_max=3    # power cycle maximum 
declare -i rst_max=3    # reset maximum 
declare -i tln_max=3    # telnet maximum
declare -i reg_max=5    # registration maximum
declare -i str_max=15   # signal strength maximum
declare -i png_max=5    # ping maximum 

declare -i cnt
declare -i curcnt

declare -i RESET_RELEASE=2
declare -i RESET_ASSERT=0  # active-low reset
declare -i POWER_OFF=0
declare -i POWER_ON=1

declare -i gpioShadowReg=0 # gpio shadow register

pauseAfterWrite=20     # pause after writing file(s)
pauseAfterPowerDn=10   # pause after power down
pauseAfterPowerUp=25   # pause after power up
pauseBeforeReset=5     # pause before reset
pauseAfterReset=25     # pause after reset
pauseAfterStatus=15    # pause after status
pauseAfterRegStatus=15 # pause after status
pauseAfterSStrength=20 # pause after signal strength
pauseAfterSsValid=10   # pause after signal strength valid

pauseBeforeReboot=120  # sleep before rebooting

pauseAfterPing=15      # pause after ping

rebootme="reboot -f"

# Google Public DNS
site="8.8.8.8"

gpio=`which setgpio`
if (( $? != 0 )) ; then
   gpio="/ctss/bin/setgpio"
fi


# ---------- Subprocesses

log ()
{
	echo "$@" 2>&1
	logger -t wgwmonitor.sh "$@"
}

copyLogs ()
{
    if [[ -f "/home/cta/debug" ]] ; then
        cp $systemLogFile "/home/cta/messages."`date -I"seconds"`
        $ctssdir/bin/purgelogs.sh
    fi
}

# updateDiagnostics()
# $1=count integer
# $2=file to update
updateDiagnostics()
{
	local -i cnt=$1
	local -i curcnt=$1
	local    file=$2

	# Read existing file. Add cnt. Update file.
	curcnt=`cat $file`
	cnt="$cnt + $curcnt"
	echo "$cnt" > $file
}


turnPowerOn()
{
	gpioShadowReg="$gpioShadowReg|$POWER_ON"
	setgpio $gpioShadowReg

	log "Power-On"
}


turnPowerOff()
{
	gpioShadowReg="$gpioShadowReg&~$POWER_ON"
	setgpio $gpioShadowReg

	log "Power-Off"
}


assertReset()
{
	gpioShadowReg="$gpioShadowReg&~$RESET_RELEASE"
	setgpio $gpioShadowReg

	log "Reset asserted"
}


releaseReset()
{
	gpioShadowReg="$gpioShadowReg|$RESET_RELEASE"
	setgpio $gpioShadowReg

	log "Reset released"
}


# ------------------------------- Begin Main -----------------------------------

log "Starting wgwmonitor ..."


# --- Clear all working count(s) ---
echo 0 > $wgwPowerFile
echo 0 > $wgwResetFile
echo 0 > $wgwTnStatusFile
echo 0 > $wgwRgStatusFile
echo 0 > $wgwSStrengthFile
echo 0 > $wgwPingFile
echo 0 > $wgwRestartFile

# Don't zero this file on startup. It gets managed in the state machine
if [[ ! -f "$wgwRebootFile" ]] ; then 
	echo 0 > $wgwRebootFile
fi

# Zero these files once on creation
if [[ ! -f "#wgwPowerDiags" ]] ; then 
	echo 0 > $wgwPowerDiags 
fi

if [[ ! -f "$wgwResetDiags" ]] ; then 
	echo 0 > $wgwResetDiags 
fi

if [[ ! -f "$wgwTnStatusDiags" ]] ; then 
	echo 0 > $wgwTnStatusDiags 
fi

if [[ ! -f "$wgwRgStatusDiags" ]] ; then 
	echo 0 > $wgwRgStatusDiags 
fi

if [[ ! -f "$wgwSStrengthDiags" ]] ; then 
	echo 0 > $wgwSStrengthDiags 
fi
if [[ ! -f "$wgwPingDiags" ]] ; then 
	echo 0 > $wgwPingDiags 
fi

if [[ ! -f "$wgwRebootDiags" ]] ; then 
	echo 0 > $wgwRebootDiags 
fi


#
# ------------------------------------------------------------------------------
#  The reset state for the gateway is power-off and reset asserted. 
#  Release the reset so the gateway will run when power is applied.
# ------------------------------------------------------------------------------
# 

state="POFF"
releaseReset

log "entering state machine"

while true ; do

	log "State($state) ..."

	case $state in 

	#
	# --- Power-Off State ---
	# Turn the Power off to the gateway. This is controlled by a GPIO pin
	# (BIT 0) on the controller.
	#
	"POFF"  )
		cnt=`cat $wgwPowerFile`

		if (( $cnt > $pwr_max )) ; then
			# power cycle count exceeded
			log "Exceeded maximum power cycle(s) ..."

			cnt=`cat $wgwRebootFile`

			# If reboot is less than max, then reboot. 
			# Otherwise just stop rebooting i.e. no help.
			if (( $cnt < $rbt_max)) ; then
				echo 0 > $wgwPowerFile

				cnt="$cnt+1"
				updateDiagnostics $cnt $wgwRebootDiags
				echo "$cnt" > $wgwRebootFile

				# don't reboot if ignore file exists
				if [[ -f $ignore ]] ; then	
					log "power cycle failed, trying REBOOT($cnt) ..."

					copyLogs
					sleep $pauseBeforeReboot
					`$rebootme`
				fi
			else
				# reboot count is exceeded, wait twice as long
				echo 0 > $wgwRebootFile
				sleep $pauseBeforeReboot
			fi
		else
			# power off the gateway
			turnPowerOff

			sleep $pauseAfterPowerDn
			state="PON"
		fi
		;;

 	#
	# --- Power-On State ---
	# Turn the Power on to the gateway.
	#
	"PON"   )
		# power on the gateway
		turnPowerOn

		cnt=`cat $wgwPowerFile`
		cnt="$cnt+1"
		updateDiagnostics $cnt $wgwPowerDiags
		echo "$cnt" > $wgwPowerFile

		log "power cycle($cnt)"

		echo 0 > $wgwResetFile
		echo 0 > $wgwTnStatusFile
		echo 0 > $wgwRgStatusFile
		echo 0 > $wgwSStrengthFile
		echo 0 > $wgwPingFile

		sleep $pauseAfterPowerUp

		log "restarting network"
		/etc/init.d/S40network restart

		state="TN"
		;;

	#
	# --- Reset-On State ---
	# Assert reset to the gateway. This is controlled by a GPIO pin (BIT 1)
	# on the controller.
	#
	"RON"   )
		cnt=`cat $wgwResetFile`

		if (( cnt >= $rst_max )) ; then
			state="POFF"
		else
			sleep $pauseBeforeReset
			assertReset
			sleep 5 # allow some time to see reset asserted

			state="ROFF"
		fi
		;;

	#
	# --- Reset-Off State ---
	# Release gateway reset
	#
	"ROFF"  )
		releaseReset

		cnt=`cat $wgwResetFile`
		cnt="$cnt+1"
		updateDiagnostics $cnt $wgwResetDiags
		echo "$cnt" > $wgwResetFile
	
		log "reset count($cnt)"

      # clear the error counts
		echo 0 > $wgwTnStatusFile
		echo 0 > $wgwRgStatusFile
		echo 0 > $wgwSStrengthFile
		echo 0 > $wgwPingFile

		sleep $pauseAfterReset
		state="TN"
		;;

	#
	# --- Telnet Session ---
	# Establish a telnet session with the gateway. There is an inherent timeout 
	# while attempting to establish a telnet session. This timeout can be adjusted
	# in the gateway status command (wgwsts.exp).
	#
	"TN"    )
		tsts=`./wgwsts.exp ts`
		
		if (( tsts > -2 )) ; then 
			# telnet session established with gateway 
			state="RS"

		else
			# telnet session failed

			cnt=`cat $wgwTnStatusFile`
			cnt="$cnt+1"
			updateDiagnostics $cnt $wgwTnStatusDiags
			echo "$cnt" > $wgwTnStatusFile

			log "telnet session failed - ts($tsts) cnt($cnt)"

			sleep $pauseAfterStatus

			# After three failures, reset the gateway
			if (( $cnt >= $tln_max )) ; then
				state="RON"
			fi
		fi
		;;

	#
	# --- Registration Status ---
	# The gateway must be registered on the network before use. A return status
	# of one means the gateway is registered.
	#
	"RS"    )
		rsts=`./wgwsts.exp rs`

		if (( rsts == 1 )) ; then
			# registration status valid
			state="SS"
		else 
			# registration status invalid

			cnt=`cat $wgwRgStatusFile`
			cnt="$cnt+1"
			updateDiagnostics $cnt $wgwRgStatusDiags
			echo "$cnt" > $wgwRgStatusFile

			log "registration status invalid($cnt) - rs($rsts)"

			sleep $pauseAfterRegStatus

			# After three failures, reset the gateway
			if (( $cnt >= $reg_max )) ; then
				state="RON"
			fi
		fi

		# Save the MEID
		imei=`./wgwsts.exp sn`
		meid=`./wgwsts.exp id`
		echo "IMEI: $imei" > $wgwEsnFile 
		echo "MEID: $meid" >> $wgwEsnFile 

		;;

	#
	# --- Signal Strength ---
	# Check the signal strength for 1xRTT(2.5G) and EVDO(3G). Either mode has
	# the data rate to handle the sign. The signal strength number is a ratio
	# of the number of bars available with three being the maximum signal 
	# strength.
	#
	"SS"    )
		ss_1xrtt=`./wgwsts.exp ss 1xrtt`
		ss_evdo=`./wgwsts.exp ss evdo`

		if (( ss_1xrtt > 33 || ss_evdo > 33 )) ; then
			# signal strength valid
			sleep $pauseAfterSsValid
			state="PNG"
		else
			# signal strength below minimum on both band(s) 

			cnt=`cat $wgwSStrengthFile`
			cnt="$cnt+1"
			updateDiagnostics $cnt $wgwSStrengthDiags
			echo "$cnt" > $wgwSStrengthFile

			log "signal strength failed($cnt) - 1xRTT($ss_1xrtt) EVDO($ss_evdo)"

			sleep $pauseAfterSStrength 

			# After three failures, reset the gateway
			if (( $cnt >= $str_max )) ; then
				state="RON"
			fi
		fi

		;;

	#
	# --- Ping ---
	# Ping a known web site (currently Google DNS).
	#
	"PNG"   )
		# Check Ping response. If PING gets response, then sleep
		rcvpackets=`./wgwping.exp $site`

		if (( $rcvpackets > 0 )) ; then
			# Ping succeeded
		 
			# Clear error counts
			echo 0 > $wgwPowerFile
			echo 0 > $wgwResetFile
			echo 0 > $wgwTnStatusFile
			echo 0 > $wgwRgStatusFile
			echo 0 > $wgwPingFile
			echo 0 > $wgwRebootFile

			sleep $pauseAfterWrite
		else
			# Ping failed

			cnt=`cat $wgwPingFile`
			cnt="$cnt+1"
			updateDiagnostics $cnt $wgwPingDiags
			echo "$cnt" > $wgwPingFile

			if (( $rcvpackets < 0 )) ; then
				log "ping failed($cnt), status($rcvpackets)"
			else	
				log "ping failed($cnt), recv'd $rcvpackets of 4 pkts"
			fi

			sleep $pauseAfterPing

			# After three failures, reset the gateway
			if (( $cnt >= $png_max )) ; then
				state="RON"
			fi
		fi
		;;

	# --- default ---
	*       )
		log "bad value $state in case"
		sleep 10
		;;

	esac # end of case "$state" in

done # while true ; do

# ---------------------------- End of Begin Main -------------------------------

