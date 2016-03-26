#!/bin/bash

TRACKER_CONF=/ctss/cfg/tracker.conf

configSerial ()
{

    echo $1 > /config/serial-number
    
    echo "Stop ID set $1."
    echo "Pause 3 sec"

    # ...restart the api routine,force re-read of config.
    /ctss/bin/stop-ctss > /dev/null
    sleep 3
    /ctss/bin/start-ctss > /dev/null
    echo "Waiting 15 sec..."
    sleep 15
    echo "You should see updated information about now..."
    echo "Display of :  .    .                   . "
    echo " Means the Serial number has not been assigned a Stop and therefore has blank data returned"
    echo " Or the server is not responding"
    sleep 5
    return 0
}

getSerialNumber()
{
  serialNumber=`cat /config/serial-number`
  while true; do
      clear
      printf "Please enter new Serial Number id (current=$serialNumber): "
      read ID
      if [[ -z "$ID" ]] ; then
      	ID="$serialNumber"
      fi
      
      printf "Is $ID the correct Serial Number? (y/n/q): " "$ID"
      read YN
      case "$YN" in
        n|N) continue ;;
        y|Y) configSerial $ID 
             return 0
             ;;
        q|x|Q|X) echo " Serial $serialNumber unchanged. "
            sleep 1 
            return 0 ;;
             
        *) echo "Invalid response. Please answer y or n"
      esac
  done
  return 0

}

printTimeDate()
{
  td=`date`
  date
  while true; do
      clear
      printf "Currently the Local Date Time set to $td \n.   (q)quit "
      read ID
      case "$ID" in
        q|Q) 
        	return 0;;
        *) echo "Invalid response. Please answer y or n"
      esac
  done
  return 0

}

getTimeZone()
{
  tz=`cat /etc/TZ`
  while true; do
      clear
      printf "Please TimeZone (chicago=CST6CDT) $tz: "
      read ID
      if [[ -z "$ID" ]] ; then
      	ID=$tz
      fi
      printf "Is $ID the correct Time Zone? (y/n):"
      read YN
      case "$YN" in
        n|N) continue ;;
        y|Y) echo "$ID" > /etc/TZ 
             return 0
             ;;
        q|Q) return 0;;
        *) echo "Invalid response. Please answer y or n"
      esac
  done
  return 0

}

setDateTime()
{
  td = `date`
  while true; do
      clear
      printf "Please enter UTC Time and Date\n (current=$td): YYYY.MM.DD-hh:mm "
      read ID
      if [[ -z "$ID" ]] ; then
      	ID=$td
      fi
      printf "Is $ID the correct Time and Date? (y/n):"
      read YN
      case "$YN" in
        n|N) continue ;;
        y|Y) TZ=UTC date -s "$ID"
        	 hwclock -u -w
        	 sleep 1;
             return 0
             ;;
        q|Q) return 0;;
        
        *) echo "Invalid response. Please answer y or n"
      esac
  done
  return 0

}

trap "" INT TERM QUIT

clear

cd /

while true; do
  clear
  printf "Configure: \n\t (v)olume:\n\t (s)erialNumber:\n\t (p)rintTimeDate:\n\t (t)imezone:\n\t (d)atetime:\n\t (q)uit: \n"
   
  read CFG
  case "$CFG" in
    v|V) /ctss/bin/volume.sh ;;
    s|S) getSerialNumber ;;
    p|P) printTimeDate ;;
    t|T) getTimeZone ;;
    d|D) setDateTime ;;
    
        #temp allow exit
    q|x|Q|X) exit 0 ;;

    #defaults
    *) echo "Invalid response. Please answer v or s" ;; 
  esac
sleep 1
done


