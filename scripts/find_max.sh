#!/bin/bash

########################################################################
# this a shell script to find the max value in several runs            #
# You should choose one ITEM which will be the target of finding max,  #
# for example, the MT SSE value.                                       #  
#                                                                      #
# Then the script runs for a endless loop                              #
# until you press CTRL+C to stop the script.                           #
#                                                                      #
# The output value will be the max value log in mat_mul                #    
#                                                                      #
# jwhust@gmail.com                                                     #
########################################################################

trap result INT

total_times=0
target_name=""
max=-100
max_log=`date +%h%d-%H%M`-mat_mul-max.log

check_env()
{
	echo "to do"
}

result()
{
	echo
	echo "Total running times is: $total_times"
	echo "Max value log is: "
	cat $max_log
	exit 0
}

run_once()
{
	# magic value again.. to avoid ruin your own log file
	./mat_mul > /tmp/mat_mul12345.tmp
	tmp=`cat /tmp/mat_mul12345.tmp | grep "$target_name" |awk -F '=' '{print $2}'`
	# shell script is unable to deal with float value, 
        # so use bc instead
	re=`echo $max\<$tmp|bc`
	if [ $re -eq 1 ] 
	then
		echo new max value found: $tmp
		max=$tmp
		cat /tmp/mat_mul12345.tmp > $max_log
	fi
}

main()
{
	while [ "$target_name" = "" ]; do
		PS3="Choose (1-4):"
		echo "Choose the max target value from the list below."
		select target_name in "ST gflop" "ST SSE gflop" "MT gflop" "MT SSE gflop"
		do
			break
		done
	done
	
	echo "You chose $target_name"
	echo "------------Mat Mul Max Find Start-------------"
	echo -e '\E[31mPress Ctrl+C to get the final result'
	echo -e '\E[0m '

	# check_env
	while true
	do
		run_once
		total_times=`expr $total_times + 1`
	done
}

main
