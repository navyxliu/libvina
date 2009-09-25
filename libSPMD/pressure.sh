#! /bin/sh


#########################################################
# this script imposed pressure to 
# libSPMD 
#########################################################

NR_CPU=`cat /proc/cpuinfo | grep "processor" | wc -l`
echo NR_CPU is ${NR_CPU}

if [ ! -e "../tpbench" ] 
then
   cd .. ; make tpbench; cd ..
fi

sudo ../tpbench -t $NR_CPU  -d 150 -m 4 -c 0 -i 20 > /dev/null 2>&1

if [ "$?" != "0" ]
then
  echo "An ERROR occured in -t $NR_CPU -c 0 case"
  exit 
else
  echo "seem anything all right"
fi
