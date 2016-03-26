#!/bin/bash 

Tags=../../tags
Version=1.0.0
CepstralTag="$Tags/cepstral-$Version"
targetdir=cepstral-uclib

if [[ ! -d $targetdir/include ]] ; then
    if [[ ! -d "$CepstralTag/uclib/cepstral_david_uclib_i386-linux_5.1.0" ]]; then
#        echo "Fool...you need the Cepstral library."
#        echo "Relax..I will go get it for you, unzip it and bring you a beer."
        pushd $Tags

        echo "svn co svn://vis/CTSS/tags/cepstral-$Version ./cepstral-$Version"
        svn co svn://vis/CTSS/tags/cepstral-$Version ./cepstral-$Version
        cd ./cepstral-$Version
        mkdir uclib
        cd uclib
        cp ../cepstral_david_uclib_i386-linux_5.1.0.tar.gz .
        tar -xvzf cepstral_david_uclib_i386-linux_5.1.0.tar.gz 
        echo "Anything else. Dear."
        popd
    fi
    
    if [[ -d "$CepstralTag/uclib/cepstral_david_uclib_i386-linux_5.1.0" ]]; then
        ln -s "$CepstralTag/uclib/cepstral_david_uclib_i386-linux_5.1.0" `pwd`"/$targetdir"
    fi
fi
