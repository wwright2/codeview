#!/bin/bash
# -------------------------------------------------------------------------
# mkflash - Build a bootable SD card from the build result.
#
# Syntax: mkflash <device-file> <mountpoint> e.g. mkflash /dev/sdb /flash
# -------------------------------------------------------------------------


die()
{
    printf "$@"
    exit 1
}

readYN()
{
	if [ -n "$PROMPT" ] ; then
	    read yn
	    case $yn in
	        y|Y) return 0 ;;
	        *)   return 1 ;;
	    esac
	else
		return 0 ;
	fi
	return 1 ; 
}

DEV=''
MOUNTPOINT='./flash'
PART='../bin/partition_layout.sfdisk'
BOOT='./rpi-2012-10-28-boot.tgz'
ROOTFS='./rpi-2012-10-28-rootfs.tgz'

PROMPT='y'

# ----------------------------------------------------------------------
# make sure this is run by root
#
[ $(id -u) -eq 0 ] || die "\nThis must be run by root\n"

# ----------------------------------------------------------------------
# check parameters
#
function usage () {
    local msg="$*";
    [ -n "$msg" ] && msg="\nError: $msg\n";
    die "$msg\n\
Syntax: $0 --dev=DEVICE [--mount=MOUNTPOINT] [--boot=BOOT] [--rootfs=ROOTFS] [--part=PARTITION_LAYOUT_FILE] [--prompt] \n\
    e.g. $0 --dev=/dev/sdc --mount=/mnt/flash --boot=boot.tar.gz -rootfs=rootfs.tar.gz --part=sfpart.file.txt\n\ 
    MOUNTPOINT defaults to ./flash \n\
    BOOT default $BOOT \n\
    ROOTFS default $ROOTFS \n\
    PARTITION_LAYOUT_FILE is an sfdisk compatible file (made with sfdisk -d) and defaults to partition_layout.sfdisk\n\
    PROMPT prompting for y/n default=y \n\
    ";
}


while [ -n "$1" ]; do
    [ "x${1:0:2}" = "x--" ] || die "Invalid argument '$1'";

    name="${1:2}";
    val=;
    [ -n "${name//[^=]/}" ] && val="${name##*=}"
    name="${name//=*}"
    
    case "$name" in 
        dev)      DEV="$val" ;;
        mount)    MOUNTPOINT="$val" ;;
        part)     PART="$val" ;;
        boot)     BOOT="$val" ;;
        rootfs)   ROOTFS="$val" ;;
        prompt)   PROMPT="$val" ;;
        *)       die "Invalid argument '$name'" ;;
    esac;
    shift;
done;

[ -n "$MOUNTPOINT" ] || usage "you must specify a mountpoint"
[ -b "$DEV"     ] || usage "Device '$dev' does not exist or is not a block device";
[ -r "$PART"    ] || usage "Partition layout file '$PART' does not exist or is not readable";

echo "--dev=$DEV ,--mount=$MOUNTPOINT, --boot=$BOOT , --rootfs=$ROOTFS , --part=$PART "

# ----------------------------------------------------------------------
# make sure the device is a base file, not a partition
#
if [[ $DEV =~ /dev/sd[a-z][0-9] ]]; then
    die "\n$DEV is not the base device file\n\n"
fi

# ----------------------------------------------------------------------
# make sure the device file is not mounted
#
mountedPartitions=$("mount")
if [[ $mountedPartitions =~ $DEV ]]; then
    die "\none or more partitions on $DEV is mounted\n\n"
fi

# --------------------------------------------
# confirmations : set Prompt.
#
if [[ "$PROMPT" != [Yy]* ]] ; then
	#echo set PROMPT null
	PROMPT=""
else
	printf "\nLAST CHANCE\n"
	printf "\nAre you SURE [$DEV] is the right partition (y/n or ^C)? "
fi

readYN || exit 0

echo "check mount point"

# --------------------------------------------
# check the mount point
#
if ! [ -d "$MOUNTPOINT" ]; then
	[ -n $PROMPT ] && printf "NOTE: Mountpoint '$MOUNTPOINT' does not exist, create it?\n" ;
	readYN  || die "Please create or specify an existing directory for the mountpoint";
    mkdir -p $MOUNTPOINT || die "Failed to create mountpoint '$MOUNTPOINT'";
fi


# --------------------------------------------
# Partition the sd card
#
[ -n "$PROMPT" ] && printf "\nDo you want to partition $DEV (y/n)? "
if readYN ; then
    
    #
    # sfdisk has problems if the partition table isn't destroyed first. Sometimes
    # it has trouble even then.
    #
    dd if=/dev/zero of=$DEV bs=1024 count=2
    sync
    sleep 1
    
    sfdisk  $DEV < $PART || die "Failed to partition '$DEV'";
    
    sleep 1
fi
	
####
# Expecting something like the following
#   Device Boot    Start       End   #sectors  Id  System
#/dev/sdb1          2048    206847     204800   b  W95 FAT32
#/dev/sdb2        206848   5941247    5734400  83  Linux
#/dev/sdb3       5941248  11675647    5734400  83  Linux
#/dev/sdb4      11675648  31115263   19439616   5  Extended
#/dev/sdb5      11677696  23146495   11468800  83  Linux
#/dev/sdb6      23148544  31115263    7966720  83  Linux

# 1=boot
# 2=rootfs   2,3 will swap back and forth on successive updates.
# 3=rootfs 
# 4=extend
#   5= data,dbase,logs
#   6= updates,config

# --------------------------------------------
# Format the partitions
#
sleep 1
[ -n "$PROMPT" ] && printf "\nDo you want to format the partitions (y/n)? "

if readYN ; then
    # done above before sfdisk called. dd if=/dev/zero of=${DEV}1 bs=512 count=1 ; sync ; sleep 1;
    printf "\n formatting... \n "
    mkfs -t vfat ${DEV}1 || die "\nError running mkfs on ${DEV}1\n"
    mkfs -t ext4 ${DEV}2 || die "\nError running mkfs on ${DEV}2\n"
    mkfs -t ext4 ${DEV}3 || die "\nError running mkfs on ${DEV}3\n"
    mkfs -t ext4 ${DEV}5 || die "\nError running mkfs on ${DEV}5\n"
    mkfs -t ext4 ${DEV}6 || die "\nError running mkfs on ${DEV}6\n"
fi

# --------------------------------------------
# Copy the rootfs and kernel
#
sleep 1

[ -d "$MOUNTPOINT/boot" ] || mkdir $MOUNTPOINT/boot
[ -d "$MOUNTPOINT/p2" ] || mkdir "$MOUNTPOINT/p2"
[ -d "$MOUNTPOINT/p3" ] || mkdir "$MOUNTPOINT/p3"
[ -d "$MOUNTPOINT/p5" ] || mkdir "$MOUNTPOINT/p5"
[ -d "$MOUNTPOINT/p6" ] || mkdir "$MOUNTPOINT/p6"

mount -t vfat ${DEV}1 $MOUNTPOINT/boot
mount ${DEV}2 $MOUNTPOINT/p2
mount ${DEV}3 $MOUNTPOINT/p3
mount ${DEV}5 $MOUNTPOINT/p5
mount ${DEV}6 $MOUNTPOINT/p6

tar -C $MOUNTPOINT/boot -xzvf $BOOT
sync
echo
sleep 1

printf "\n...This is coping 1.45G of data to Partition 2 of the SD card...be patient 50min \n"
tar -C $MOUNTPOINT/p2 -xzvf $ROOTFS
sync
printf " Done..tar rootfs to partition."
sleep 1

touch $MOUNTPOINT/p2/forcefsck
touch $MOUNTPOINT/p3/forcefsck
touch $MOUNTPOINT/p5/forcefsck
touch $MOUNTPOINT/p6/forcefsck
mkdir -p $MOUNTPOINT/p6/update/tmp

umount $MOUNTPOINT/boot
umount $MOUNTPOINT/p2
umount $MOUNTPOINT/p3
umount $MOUNTPOINT/p5
umount $MOUNTPOINT/p6

fsck.vfat -y ${DEV}1
fsck -y ${DEV}2
fsck -y ${DEV}3
fsck -y ${DEV}5
fsck -y ${DEV}6

echo "..Completed..You can remove SD card.";
