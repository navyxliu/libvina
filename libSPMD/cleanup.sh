#! /bin/bash
#this script is a helper tool to do cleanup stuff.
#in case my program does not release system resource
#navy.xliu@gmail.com
#10/5 2009

#kill all threads derived from tpbench
#sudo killall -q $1
#../kill_them.sh $1

#rm all log files
rm -f timelog*

echo "ipcrm"

#release all requested semaphores
#use ipcs to check out this information
#to get the requirement of sem, refer to `cat /proc/sys/kernel/sem`
for s in `ipcs -s | awk '{if (NR > 3) print $2}END{print end}'`
do
   sudo ipcrm -s $s
done
