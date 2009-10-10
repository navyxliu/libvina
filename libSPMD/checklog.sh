#! /bin/sh
echo "leaders"
ls -l timelog*ldr | awk '{if ($5 != 0 ) print $0}'

echo
echo "tasks"
ls -l timelog*tsk | awk '{if ($5 != 0 ) print $0}'
