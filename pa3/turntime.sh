#!/bin/bash
num=0
for typ in cpu io mixed
do
for sched in SCHED_OTHER SCHED_FIFO SCHED_RR
do
num=`sudo ./pa3 10 $sched $typ turntime`
num=${num%?}
for ((i=0; i<9; i++))
do
temp=`sudo ./pa3 10 $sched $typ turntime`
num=`echo $num`+`echo ${temp%?}`
done
echo 'For' $sched 'type' $typ 'The Turnaround Time is:'
echo '('${num%?}')/100' | bc -l
done
done
