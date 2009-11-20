#! /bin/bash
#when debuging the library, my programs might accidently generate a lot of zombies.
#this script kill them all.navy.xliu@gmail.com
#2009/09/20
for p in `ps aux | grep $1 | awk -F ' ' '{print $2}' `
do
  kill -9 $p
done

