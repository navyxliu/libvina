#!/bin/bash

clean_loops()
{
	rm vina.loops_per_ms
}

if [ "$1X" == "1X" ]; then
    echo "recalc vina.loops_per_ms"
    rec=1
else
    echo "skip recalc"
    rec=0
fi

if [ "$2X" == "1X" ]; then
    echo "pre-sleep"
    pre=1 
else
    echo "skip pre-sleep"
    pre=0
fi

for i in 1 4 10 50 100 200 400 1000 1500 2000 5000 10000
do
  ./test_ck_burning $i | grep "result:"

# re-calc vina.loops_per_ms or not
  if [ "$1X" == "1X" ]; then
    clean_loops
  fi

# sleep or not
  if [ "$2X" == "1X" ]; then
    sleep 1
  fi
done
