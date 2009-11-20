#!/bin/bash

#manage cpus for multicore envirement, need root privilege
#navy.xliu@gmail.com
#Nov.6, 2009
#thanks for jwhust@gmail.com

#usage: cores [-cr] [Numbers,...]
# -c: close
# -r: reset
# ingore numbers can display current configuration of cpus, but do nothing

val=1
max_cpu=`cat  /sys/devices/system/cpu/possible | awk -F "-" '{print $2}'`
#echo "possible cpus: " ${max_cpu}

#reset all cores
if [ "$1X" == "rX" ]; then
  i=1
  while [ $i -le ${max_cpu} ]
  do
    echo 1 > /sys/devices/system/cpu/cpu$i/online
    i=$(($i+1))
  done
  shift
fi

if [ "$1X" == "cX" ]; then
   #c[lose] close the specific cores
   val=0
   shift
fi


for p in "$@"
do
echo "${val}" > /sys/devices/system/cpu/cpu$p/online
done

echo "current cpus: " `cat /sys/devices/system/cpu/online`
