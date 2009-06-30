#!/bin/sh
#This file not supposed to be distributed.
#This file can only be executed by root.

#Setup libstdc++ for customized gcc 4.4.0 and libstdc++.
#This is required only for -O2 or higher flags.
#export LD_PRELOAD=/usr/local/lib64/libstdc++.so.6
export LD_LIBRARY_PATH=/usr/local/lib64:$LD_LIBRARY_PATH

#Enable Low level cache misses event.
#  1.Observe all cores, not only registered processor. This is only supported by core duo and later processors.
#otherwise, processor will simply ignore it.
#  3.Observe user applications only. to include OS affect, using 0xc3412e
#for more information, see [profiler.cc]
sudo /usr/sbin/wrmsr 390 0xc1412e
sudo chmod -o+r /dev/cpu/0/msr
