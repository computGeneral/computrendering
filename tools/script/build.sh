#!/bin/bash

# CG1 GPU Simulator — Top-level build script
# This script configures and builds the project into the _BUILD_ directory.
#
# Usage (from anywhere):
#   bash tools/script/build.sh [ArchModelDevel]
#     ArchModelDevel = 0 (default) → archmodel disabled
#                      1           → archmodel development enabled

ArchModelDevel=0

if [ -z "$1" ]; then
    echo "[INFO] CG_ARCH_MODEL_DEVEL MODE NOT SPECIFIED, (Default: Disabled)"
else
    echo "[INFO] CG_ARCH_MODEL_DEVEL MODE SPECIFIED $1"
    ArchModelDevel="$1"
fi

osTyp="$(uname -s)"

# Note: On Windows (MSVC), we only generate the build files.
#       On Unix-like systems we also invoke make.
if [[ "$osTyp" == "Win" || "$osTyp" == *"MINGW"* ]]; then
    cmake -S ./ -DCMAKE_BUILD_TYPE=Debug -DCG_ARCH_MODEL_DEVEL="$ArchModelDevel" -DMSVC=1 -A "Win32" -B _BUILD_
else
    cmake -S ./ -DCMAKE_BUILD_TYPE=Debug -DCG_ARCH_MODEL_DEVEL="$ArchModelDevel" -B _BUILD_
    cd _BUILD_ || exit 1
    make -j 4
    cd ..
fi

