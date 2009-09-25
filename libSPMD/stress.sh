#! /bin/sh
#########################################################
# this script imposed stress test to libSPMD.
#########################################################

NR_CPU=`cat /proc/cpuinfo | grep "processor" | wc -l`
echo NR_CPU is ${NR_CPU}

if [ ! -e "../tpbench" ] 
then
   make tpbench;
   if (($? != 0)) 
   then
     echo "failed to build tpbench"
     exit 
   fi
fi

echo "start to stress TEST"

for ((i=1; i<=${NR_CPU}; i=i<<1)); do
    echo "testing -t$i ..."
    sudo ./tpbench -t$i -d 150 -m 4 -c9 -i 20 > /dev/null 2>&1
    if [ "$?" != "0" ]
    then
       echo "An ERROR occured in -t $i -c 0 case"
       exit 
    fi
done

echo "stress TEST passed"
