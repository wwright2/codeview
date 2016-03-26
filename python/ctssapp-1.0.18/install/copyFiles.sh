#!/bin/bash
# Read installFiles.txt, copy files from svn to output directory
#

set -x

if [ ! $# -eq 1 ]; then
    BUILDROOT_VER=
else
    BUILDROOT_VER=$1
fi

getBuildrootDir ()
{
    top="$1"
    ver="$2"

    tst="$top/../buildroot-$ver"

    if [ -d "$tst" ]; then
        echo buildroot-$BUILDROOT_VER
    else
        echo buildroot
    fi
}

getTopDir ()
{
    set -x
    p=$(pwd)

    if [[ $p =~ ^(.*/tags/ctssapp-[0-9]+\.[0-9]+\.[0-9]+).*$ ]]; then
        top=${BASH_REMATCH[1]}
        echo $top
	return
    fi

    if [[ $p =~ ^(.*/branches/ctssapp-[0-9]+\.[0-9]+\.[0-9]+).*$ ]]; then
        top=${BASH_REMATCH[1]}
        echo $top
	return
    fi

    if [[ $p =~ ^(.*/ctss)/.* ]]; then
        top=${BASH_REMATCH[1]}
        echo $top
	return
    fi
}


TOPDIR=`getTopDir`
BRDIR=$(getBuildrootDir $TOPDIR $BUILDROOT_VER)
echo ===================================
echo TOPDIR = [$TOPDIR]
echo BRDIR  = [$BRDIR]
INSTALL=$TOPDIR/target/ctss/install
echo INSTALL = $INSTALL
echo ===================================
BIN=${TOPDIR}/target/ctss/bin
INST_FILES=./installFiles.txt
INST_SCRIPT=./install.sh

echo trying to create $INSTALL
if [ ! -d $INSTALL ]; then
    mkdir -p $INSTALL
    if [ ! -d $INSTALL ]; then
        echo could not create $INSTALL
        exit 1
    fi
fi

if [ ! -d $BIN ]; then
    echo $BIN does not exist
    exit 1
fi

cp $INST_FILES "$INSTALL"
cp $INST_SCRIPT "$BIN"

while read src dst; do
    [[ $src = \#* || $src = '' ]] && continue
    src=$(echo $src | sed 's!BUILDROOT!../'${BRDIR}'!')
    src=$(echo $src | sed 's!BUILDROOT_PKG!../'${BUILDROOT_PKG}'!')
    src="$TOPDIR/$src"
    echo copying ["$src"] to ["$INSTALL"]
    cp "$src" "$INSTALL"
done < $INST_FILES
