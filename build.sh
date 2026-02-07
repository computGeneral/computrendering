#!/bin/bash

ArchModelDevel=0

if [ -z $1 ]; then
    echo "[INFO] CG_ARCH_MODEL_DEVEL MODE NOT SPECIFIED, (Default: Disabled)"
else
    echo "[INFO] CG_ARCH_MODEL_DEVEL MODE SPECIFIED $1"
    ArchModelDevel=$1
fi


osTyp=`uname -s`

#if [ "$osTyp" == "Linux" or "$osTyp" == "Darwin" ]; then
if [[ "$osTyp" == "Win" || "$osTyp" == *"MINGW"* ]]; then
    cmake -S ./ -DCMAKE_BUILD_TYPE=Debug -DCG_ARCH_MODEL_DEVEL=$ArchModelDevel -DMSVC=1 -A "Win32" -B _BUILD_
else
    #cmake -S driver/utils/OGLApiCodeGen -B _BUILD_/driver/utils/OGLApiCodeGen 
    #cd _BUILD_/driver/utils/OGLApiCodeGen
    #make -j 4
    #cd -
    cmake -S ./ -DCMAKE_BUILD_TYPE=Debug -DCG_ARCH_MODEL_DEVEL=$ArchModelDevel -B _BUILD_ 
    cd _BUILD_
    make -j 4
fi

