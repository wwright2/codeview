#!/bin/bash
# -------------------------------------------------------------------------
# ctss-install.sh 
#
# This needs to be kept in sync with updateFiles.txt. This assumes
# that the files named in updateFiles.txt are copied into the output
# /ctss/install directory, and so are included in the final update
# package file. The updateFiles.txt file is also included in the
# update because it contains the target path on the sign.
#
# This file should accumulate changes. That is, if version 1 deletes a
# file in the root file system, so should version 2. It should be
# runnable on any version of the sign software, and bring that version
# up to the current one.
#
# Note that we use the ctssFallback filesystem. That's where new
# installations are put before the partitions are swapped.
#
# Be very careful about changing the root file system. (Duh!)
# -------------------------------------------------------------------------

INST_FILES=/ctssFallback/install/installFiles.txt
BASEDIR=/ctssFallback/install

#
# Add new or changed files
#

exec > /home/cta/install.log
exec 2>&1

echo  install.sh started at $(date) in $(pwd)
while read src dst; do
    [[ $src = \#* || $src = '' ]] && continue
    f=${src##*/}
    echo copying ["$BASEDIR/$f"] to ["$dst"]
    cp "$BASEDIR/$f" "$dst"
done < "$INST_FILES"

echo Setting ownership of /etc/sysconfig.sh
chown provision /etc/sysconfig.sh

echo Setting ownership of /ctss and /ctssFallback
chown -R luminator:luminator /ctss
chown -R luminator:luminator /ctssFallback
chown -R luminator:luminator /config

echo install.sh ending at $(date) in $(pwd)
exit 0
