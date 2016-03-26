#!/bin/bash
#
# file pppmonitor.sh
#
#[root@LTG ~]# netstat -rn
#Kernel IP routing table
#Destination     Gateway         Genmask         Flags   MSS Window  irtt Iface
#255.255.255.255 0.0.0.0         255.255.255.255 UH        0 0          0 eth1
#66.174.184.64   0.0.0.0         255.255.255.255 UH        0 0          0 ppp0
#192.168.2.0     0.0.0.0         255.255.255.0   U         0 0          0 eth1
#192.168.0.0     0.0.0.0         255.255.255.0   U         0 0          0 eth0
#0.0.0.0         0.0.0.0         0.0.0.0         U         0 0          0 ppp0
#
#
# ps -ef |grep -v grep | grep `cat /var/run/ppp0.pid`
#


ctssdir="/ctss"



# ---------- File names.

pppup="ppp-on.sh"
pppdown="ppp-off.sh"

pppCntFile="/ctss/ppp-restart.cnt"
modemResetFile="/ctss/modemReset.cnt"
rebootCntFile="/ctss/reboot.cnt"
systemLogFile="/var/log/messages"

pppDiagnostics="/home/cta/pppStarts.txt"
rebootDiagnostics="/home/cta/reboots.txt"
modemResetDiagnostic="/home/cta/modemRestarts.txt"


# ---------- Include
source $ctssdir/bin/modemdev.sh



ignorePPP="$ctssdir/.ignore"

# ---------- Variables ---

declare -i cnt
declare -i curcnt
declare -i rbt
declare -i modemResetCnt
declare -i maxppp
declare -i maxreboot

maxppp=2
maxreboot=3

sleeptimePause=20
sleeptimePPPdn=8
sleeptimePPPup=21
sleepbeforeReboot=120
rebootme="reboot -f"

snt=''
rcv=''

# google public dns
site="8.8.8.8"


gpio=`which setgpio`
if (( $? != 0 )) ; then
   gpio="/ctss/bin/setgpio"
fi


# ---------- Subprocesses

copyLogs ()
{
    if [[ -f "/home/cta/debug" ]] ; then
        cp $systemLogFile "/home/cta/messages."`date -I"seconds"`
        $ctssdir/bin/purgelogs.sh
    fi
}


testNetstat ()
{
    netstat -rn |grep ppp0
    echo $?
}


pingSite ()
{
	
	`ping -c 3 -w 3 $site | grep packets > /tmp/ping.txt`
	snt=`cat /tmp/ping.txt | cut -d',' -f1`
	rcv=`cat /tmp/ping.txt | cut -d',' -f2`
	#trim
	snt=`echo $snt`
	rcv=`echo $rcv`
	# set sent/recieve
	snt=`echo $snt |cut -d' ' -f1`
	rcv=`echo $rcv |cut -d' ' -f1`
	# return received.
	return $rcv
}


# ----------- Begin Main



# Begin Monitor..
#  - if ppp up
#          sleep ; wait check later
#  - if ppp down
#        if pppCnt >= 1
#            resetModem()
#        else
#            cycle ppp off/on; pppCnt++
#
#        after cycling pppOff pppOn then reset Modem
#  -  

cd $ctssdir


# GPIO - Modem kill switch, boots up reads gpio as Powered OFF signal.
# .. Start pppMonitor and Power UP the modem

$pppdown
sleep 2
setgpio $modemOn

modemdev > modemDev
waitForDevice

# If IGNORE file Does NOT exist then start pppd
if [[ ! -f $ignorePPP ]] ; then
    $pppup
	sleep 12
fi


# initialize ppp reset Count, modem reset Count, 
#  have to trust rebootCnt, gets reset if we get a URL request back or ppp comes up.
echo 0 > $pppCntFile
echo 0 > $rebootCntFile

while true ; do

    # 
    #  IGNORE PPP0
    #
    if [[ -f $ignorePPP ]] ; then
        # Yes, sleep
        #echo $? ;
        echo "ppp up"

        
        #..Clear counts
        echo 0 > $pppCntFile
        echo 0 > $modemResetFile
        echo 0 > $rebootCntFile
        sleep $sleeptimePause

    #
    # is PPP0 in routing table Yes, then ping,validate link passes data.
    # 
    elif [[ `netstat -rn |grep ppp0` ]] ; then

        # Check Ping response...if ppp0 exist and we get PING responses then OK. Sleep. else reset.
        pingSite
        let rcvpackets=$?
        echo $rcvpackets

        if (( $rcvpackets != 0 )) ; then
            # Yes, sleep
            #echo $? ;
            echo "ppp up"
	       
            #..Clear counts
            echo 0 > $pppCntFile
            echo 0 > $modemResetFile
            echo 0 > $rebootCntFile
            sleep $sleeptimePause
        else
            # DOWN
            $pppdown
            sleep 4
            /ctss/bin/modemstatus.sh > /home/cta/modemstatus.txt 2>&1
            sleep $sleeptimePPPdn
            cnt=`cat $pppCntFile`
            # UP
            $pppup
            sleep $sleeptimePPPup
            cnt="$cnt+1"
            updateDiagnostics $cnt $pppDiagnostics
            echo "$cnt" > $pppCntFile
        fi
        
        
    else
    
        # No, reset ppp
        echo " ppp down RESTART"
        logger "ppp down RESTART"
        # DOWN
        $pppdown
        sleep 4
        /ctss/bin/modemstatus.sh > /home/cta/modemstatus.txt 2>&1
        sleep $sleeptimePPPdn

        cnt=`cat $pppCntFile`
        
        if (( $cnt > 1 )) ; then
           echo "reset the modem"
           resetModem
        fi

        # UP
        $pppup
        sleep $sleeptimePPPup

        cnt=`cat $pppCntFile`
        cnt="$cnt+1"
        updateDiagnostics $cnt $pppDiagnostics
        echo "$cnt" > $pppCntFile
        
        # if ppp restart more than twice then try a reboot.
        if (( $cnt > $maxppp )) ; then
            rbt=`cat $rebootCntFile`
            # if reboot is less than 3 then reboot. else...just stop rebooting i.e. no help.
            if (( $rbt < $maxreboot )) ; then
                rbt="$rbt+1"
                updateDiagnostics $cnt $rebootDiagnostics
                echo "$rbt" > $rebootCntFile
                echo "...ppp failed trying reboot...."
                logger "...restart PPP failed trying REBOOT..."
                
                echo 0 > $pppCntFile
                copyLogs
                sleep $sleepbeforeReboot
                `$rebootme`
            fi
        fi
    fi
done

