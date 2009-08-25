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
  echo "usage $1: -t [thread] -d [delay]"
  exit -1
fi
cmd="-t $1 -d $2"
c_range="0,1,2,3,4,5,6,7,8,9,10"
m_range="1"
iter=12
log=t4d30-`date +%h%d-%H%M`.log

for m in 1 2 4; do
for c in 0 1 2 3 4 5 6 7 8 9 10; do
    #echo ../tpbench $cmd -c $c -m $m -i $iter
    sudo ../tpbench $cmd -c $c -m $m -i $iter > tmp
    cat tmp | grep "group overhead Expect is" | awk -F ' ' '{print $5}'
    cat tmp >> $log
done 
echo >> $log
echo
done