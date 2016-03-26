#!/bin/bash
# -----------------------------------------------------------------------
# mkUpdatePackage.sh - Create the update package from the target
# directory, sign it and create the final distribution tarball.
# -----------------------------------------------------------------------

# Major Minor version.
if [[ -z "$1" ]] ; then
   echo "We expect mmVersion to be passed from make.conf"
   exit 1
fi

mmVersion=$1
version=
curDir=`pwd`



die ()
{
    echo "$@"
    exit 1
}

[ ! -d ./target/ctss ] && die "Can't find ctss directory"

getTagNumber ()
{
    parent=$(pwd)
    parent=${parent##*/}
    [ -z "$parent" ] && die "What are you doing in the root directory?"
    if [[ $parent =~ .*([0-9]+\.[0-9]+)\.[0-9]+.* ]];then
        echo ${BASH_REMATCH[1]}
    else
        echo ''
    fi
}

curDir=${curDir##*/}
if [[ $curDir =~ ^ctssap.*([0-9]+\.[0-9]+\.[0-9]+).* ]]; then
    #
    # This is in a directory under tags, get the version number from
    # the name of the current directory.
    #
    version=${BASH_REMATCH[1]}

else

    mmVersion=$(getTagNumber)
    if [ -z "$mmVersion" ];then
        mmVersion=1.0
    fi
    
    #
    # The build number comes from subversion, but it can have stuff other
    # than digits in it. If there are other characters make sure they
    # don't survive this process.
    #
    build=$(svnversion)
    if [ -z $build ]; then
        build=0
    else
        if [[ $build =~ ([0-9]+).* ]]; then
            build=${BASH_REMATCH[1]}
        else
            build=0
        fi
    fi

    version=${mmVersion}.${build}
fi

echo $version > target/ctss/cfg/ctss-version

#
# Do not tar up the directory itself, go one level lower and tar the
# contents. That way we can install it on the backup partition before
# making the switch.
#

UPDATE=ctssUpdate-${version}.tar.bz2
PKG=ctssUpdatePkg-${version}.tar.bz2

cd ./target/ctss
tar cvjf ../${UPDATE} .
cd ..

sha1sum ${UPDATE} > dist.sum
[ $? -ne 0 ] && die "Error creating sha1sum"

openssl rsautl -sign -inkey ../ctssPrivateKey.pem -passin pass:asdf -in dist.sum -out dist.sig
[ $? -ne 0 ] && die "Error signing update tarball"

tar cvjf ../$PKG dist.sig ${UPDATE}
[ $? -ne 0 ] && die "Error creating final tarball"

exit 0
