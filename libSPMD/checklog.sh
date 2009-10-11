#! /bin/bash

if [ "$1X" = "X" ]; then
	echo "leaders"
	ls -l timelog*ldr | awk '{if ($5 != 0 ) print $0}'

	echo
	echo "tasks"
	ls -l timelog*tsk | awk '{if ($5 != 0 ) print $0}'
else
#	echo you want to find target $1, results:
	for i in `ls timelog*`
	do
		head -n 1 $i |grep $1 > /dev/null
		if [ $? -eq 0 ]; then
			echo $i
		fi
	done
fi
