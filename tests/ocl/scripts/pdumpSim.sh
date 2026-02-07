#!/bin/bash

if [ -z $1 ]; then
    echo "[ERRO] please enter the test name we want to verify"
    echo "[ERRO] ./pdumpSim.sh <caseName>"
    echo "[ERRO] i.e. >>./pdumpSim.sh ocl_template"
    exit
fi

oclEnvScrPath="$(pwd)"


CSIM_PATH="${oclEnvScrPath}/../../xmd_env/utils/csim"

export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${CSIM_PATH}

cd ../_BUILD_

# --usc-verbose \

${CSIM_PATH}/sim \
 --in-dir=${oclEnvScrPath}/../_BUILD_/pdump/pdump_capt_$1 \
 --config-dir=${CSIM_PATH} \
 --fb-bitmap-output \
 --watchdog-disable \
 --force-perf-counters \
 --counter-dump-mode segment \
 > ./runsim.log
