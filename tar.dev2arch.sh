#!/bin/bash
#
#!/bin/bash
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 
   exit 1
fi

readYN()
{
    read yn
    case $yn in
        y|Y) return 0 ;;
        *)   return 1 ;;
    esac
}
die()
{
    printf "$@"
    exit 1
}


# Default action
#  tar -C sd -czvf v1.0.0.boot.tgz .
#

img="v1.0.0.$$.tgz"
dev=""
mountpt="./sd"

if [[ -n "$2" ]] ; then
   img="$2"
fi

while [ -z $dev ] ; do 
  	if [[ -n "$1" ]] ; then
    	dev="$1"
	else
	    echo "device= e.g. /dev/sd<drive><partition>  where drive=b,c, and partition=1,2,3,4  remove <><>  "
		read -p "Enter device and partition [$dev]: " REPLY
		echo "device is $REPLY"
	fi
    printf "mount $dev $mountpt \n tar -C $mountpt -cvzf $img \nContinue? (y/n or ^C) "
	readYN || die "select nN \n"
done



# ----------------------------------------------------------------------
# make sure the device file is not mounted
#
mountedPartitions=$("mount")
if [[ $mountedPartitions =~ $dev ]]; then
    die "\none or more partitions on $dev is mounted\n\n"
fi

# --------------------------------------------
# confirmation 
#
printf "\nLAST CHANCE\n"
printf "\nAre you SURE [$dev] is the right partition create file $img  (y/n or ^C)? "
readYN || die "you exited \n"

echo "Here we go..."
echo "tar -C $mountpt -czvf $img ."
mount $dev $mountpt
tar -C $mountpt -czvf $img .

sleep 1
sync

sleep 1
umount $dev

