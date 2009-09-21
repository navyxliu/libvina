#! /bin/bash

echo checking...
cp test_parameter.def test_parameter
make $1
./$1 > /dev/null

if [ $? -eq 0 ] 
then
    echo okay.
else
    echo wrong answer.
    exit
fi

#sed -in 's/#-D__NDEBUG/-D__NDEBUG/g' test_parameter

echo testing...
for g in 64 128 256 512; do 
    for k in 2 4 8 16 32; do
	let n=$g*$k
	while [ $n -lt 1024 ]; do 
	    let n=n*$k
	done

	if [ $n -gt 4096 ]
	then
	    continue
	fi
	echo $g $k $n

	sed -in -e "s/-DMM_TEST_GRANULARITY=[0-9]*/-DMM_TEST_GRANULARITY=$g/g" \
	    -e "s/-DMM_TEST_SIZE_N=[0-9]*/-DMM_TEST_SIZE_N=$n/g" \
	    -e "s/-DMM_TEST_K=[0-9]*/-DMM_TEST_K=$k/g" test_parameter
	
	make $1
	./$1 >> out.txt
    done

done
cp test_parameter.def test_parameter
echo finish, check out.txt

