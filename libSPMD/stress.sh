#! /bin/sh
#########################################################
# this script imposed stress test to libSPMD.
#########################################################

NR_CPU=`cat /proc/cpuinfo | grep "processor" | wc -l`
echo NR_CPU is ${NR_CPU}

if [ ! -e "./tiny" ] 
then
   make
   if (($? != 0)) 
   then
     echo "failed to build tpbench"
     exit 
   fi
fi

echo "start to stress TEST"
#NR_CPU=4
for ((i=${NR_CPU}; i>=1; i=i>>1)); do
for ((j=1; j<=200; ++j)); do
    echo "testing -t$i $j ..."
    #sudo ./tpbench -t$i -d 1000 -m 4 -c10 -i 20 #> /dev/null
    sudo ./tiny $i > /dev/null #2>&1
    if [ "$?" != "0" ]
    then
       echo "An ERROR occured in -t $i -c 10 case"
       exit 
    else
       sudo ./cleanup.sh tiny
    fi
done
done

echo "stress TEST passed"
