# This a script of testing tp, mt::thread, libSPMD overhead
# jwhust@gmail.com
#
# input: if no specified d and t, 
#        use default value
#
# output: in a format that suitable for ploting
#
#!/bin/bash

d_range="1,10,30,50,100,200,500,1000"
t_range="1,2,4,8,12,16,20,24,28,32"

tab=$'\t'

group_result=/tmp/test_threadlib.group.result
thread_result=/tmp/test_threadlib.thread.result
tmplog=/tmp/test_threadlib.tmp.log

help_info()
{
		echo "<usage:> ./`basename $0` [-t (group array)] [ -d (dalay array)]"
		echo "Example(Default):"
		echo "    ./`basename $0` -t \"${t_range}\" -d \"${d_range}\""
}

set_up()
{
	while getopts 't:d:h' OPT; do
		case $OPT in
		h) 
			help_info;;
		d)
			if [ "$OPTARG" != "" ]; then
				d_range="$OPTARG"
			fi;;
		t)
			if [ "$OPTARG" != "" ]; then
				t_range="$OPTARG"
			fi;;
		?)
			echo "wrong input!"
			echo "Read usage with `basename $0` -h"
		esac
	done

	echo ------------------Test Start------------------
	echo d value is: $d_range
	echo t value is: $t_range
}

clean_up()
{
	rm $tmplog
	t=`date +%h%d-%H%M`
	mv $group_result group_result_$t.log
	mv $thread_result thread_result_$t.log
	echo group result is in group_result_$t.log
	echo thread result is in thread_result_$t.log
}

save_result_group()
{
	mt=`cat $tmplog |grep "One group overhead" |head -n 1|awk -F ' ' '{print $5}'`
	tp=`cat $tmplog |grep "One group overhead" |tail -n 1|awk -F ' ' '{print $5}'`
	# add libSPMD here

	echo "$1${tab}${mt}${tab}${tp}"  >> $group_result
}

save_result_thread()
{
	mt=`cat $tmplog |grep "amortized thread cost" |head -n 1|awk -F ' ' '{print $4}'`
	tp=`cat $tmplog |grep "amortized thread cost" |tail -n 1|awk -F ' ' '{print $4}'`
	# add libSPMD here

	echo "$1${tab}${mt}${tab}${tp}"  >> $thread_result
}


test_body()
{
	for d in `echo $d_range|sed 's/,/ /g'`; do
		# log header
		echo "------start test d=$d while changing t-----" >> $group_result
		echo "t${tab}mt${tab}tp" >> $group_result
		echo "------start test d=$d while changing t-----" >> $thread_result
		echo "t${tab}mt${tab}tp" >> $thread_result

		# start to get the data
		for t in `echo $t_range|sed 's/,/ /g'`; do
			./tpbench -t $t -d $d > $tmplog
			# each line of following is a way of handling the result
			save_result_group $t
			save_result_thread $t
		done

		# log tailer
		echo "------end testing d=$d------" >> $group_result
		echo >> $group_result
		echo "------end testing d=$d------" >> $thread_result
		echo >> $thread_result
	done
}


main()
{
	# help_info
	set_up $@
	test_body
	clean_up
}

main $@
