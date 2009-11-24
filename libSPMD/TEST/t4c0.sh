#!/bin/sh
#this script is [t4c0.sh]
#it tests thread performance in this order:
# 1. mt -- pthread impl.
# 2. pt -- thread pool 
# 3. wp -- libspmd 
#for each group

if [ $# -lt 2 ]
then
  echo "usage $1: -d [delay] -i [iteration]"
  exit -1
fi
cmd="-d $1 -c 0 -t 4"

iter=$2
log=t4c0-`date +%h%d-%H%M`.log

for m in 1 2 4; do
    #echo ../tpbench $cmd -t 4 -m $m -i $iter
    sudo ../tpbench $cmd -m $m -i $iter > tmp
    cat tmp | grep "group overhead Expect is" | awk -F ' ' '{print $5}'
    cat tmp >> $log
    sleep 5
echo >> $log
echo
done
