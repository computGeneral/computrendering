

if [ -z $1 ]; then
    echo "[ERRO] please specify the case binary name located in _BUILD_"
    echo "[ERRO] ./pdumpCap.sh <caste binary located in root/_BUILD_>"
    echo "[ERRO] i.e. >>./pdumpCap.sh ocl_template"
    exit
fi



#  # pdump works in singleton mode
#  pdump_thread=$(pgrep pdump)
#  if [ "$pdump_thread"x != ""x ]; then
#      echo "someone else is running pdump, exit !!!"
#      exit
#  fi
#  
#  # reset driver
#  export PVR_SRVKM_PARAMS="GeneralNon4KHeapPageSize=0x010000"
#  stop_rez=$(/etc/init.d/rc.musa stop | grep Unloaded)
#  if [ "$stop_rez"x == ""x ]; then
#      echo "system is down, will reboot the machine, please connetct in 2mins !!!"
#      echo 1 >/proc/sys/kernel/sysrq
#      echo b >/proc/sysrq-trigger
#      exit
#  fi
#  /etc/init.d/rc.musa start


mkdir -p ../_BUILD_/pdump

echo "$1"

python3 ../../xmd_env/utils/tcap.py -c "../_BUILD_/$1" \
	                            -o "../_BUILD_/pdump/"
