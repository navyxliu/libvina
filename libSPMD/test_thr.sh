#! /bin/bash
#########################################################
# this script imposed stress test to libSPMD.
#########################################################

NR_CPU=`cat /proc/cpuinfo | grep "processor" | wc -l`
echo NR_CPU is ${NR_CPU}

if [ ! -e "../tpbench" ] 
then
   make ../tpbench
   if (($? != 0)) 
   then
     echo "failed to build tpbench"
     exit 
   fi
fi

delay=10000
if [ $# > 0 ]
then
  delay=$1
fi

if [ $# > 1 ]  && [ '$2' = 'q' ]
then
  quiet=">/dev/null"
else
  quiet=""
fi

echo "start to stress TEST"
for ((i=${NR_CPU}; i>=1; i=i>>1)); do
for ((j=1; j<=50; ++j)); do
    echo "testing -t$i $j ..."
    ../tpbench -t$i -d${delay} -m 4 -c10 -i 20 ${quiet}
    if [ "$?" != "0" ]
    then
       echo "An ERROR occured in -t $i -c 10 case"
       exit 
    else
       ./cleanup.sh tpbench
    fi
done
done

echo "stress TEST passed"
