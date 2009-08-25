#!/bin/sh
#this script is [t4d30.sh]
#it tests thread performance in this order:
# 1. mt -- pthread impl.
# 2. pt -- thread pool 
# 3. wp -- libspmd 
#for each group
#group overhead Expect is    98.00 us
#group overhead Variance is    19.33
#group overhead Standard Error is     4.40
if [ $# -lt 2 ]
then
  echo "usage $1: -d [delay] -c [contend]"
  exit -1
fi
cmd="-d $1 -c $2"

iter=12
log=d30c5-`date +%h%d-%H%M`.log

for m in 1 2 4; do
    for t in 1 2 4 8 16 32 64 128; do
    echo ../tpbench $cmd -t $t -m $m -i $iter
    sudo ../tpbench $cmd -t $t -m $m -i $iter > tmp
    cat tmp | grep "group overhead Expect is" | awk -F ' ' '{print $5}'
    cat tmp >> $log
done 
echo >> $log
echo
done