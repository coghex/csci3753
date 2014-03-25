#!/bin/bash

echo '\begin{center}'
echo '\begin{tabular}{|c|c|c|c|c|}'
echo '\hline'
echo 'Schedule & Utilization & Type & Trial & Execution Time\\'
echo '\hline'
echo '\hline'


for sched in SCHED_OTHER SCHED_FIFO SCHED_RR
do
unset num
unset temp
for typ in cpu io mixed
do
echo -ne $sched
echo -ne ' & Low & '
echo -ne $typ
echo -ne ' & 1 & '
num=`sudo ./pa3 10 $sched $typ extime`
echo $num '\\'
echo '\cline{4-5}'
for ((i=0; i<9; i++))
do
temp=`sudo ./pa3 10 $sched $typ extime`
echo ' & & &' $((i+2)) '&' $temp '\\'
echo '\cline{4-5}'
num=`echo $num`+`echo $temp`
done
temp=`echo 'scale=6; ('$num')/10' | bc -l`
echo ' & & & avg &' $temp '\\'
echo '\hline'
done

unset num
unset temp
for typ in cpu io mixed
do
echo -ne $sched
echo -ne ' & Medium & '
echo -ne $typ
echo -ne ' & 1 & '
num=`sudo ./pa3 50 $sched $typ extime`
echo $num '\\'
echo '\cline{4-5}'
for ((i=0; i<9; i++))
do
temp=`sudo ./pa3 50 $sched $typ extime`
echo ' & & &' $((i+2)) '&' $temp '\\'
echo '\cline{4-5}'
num=`echo $num`+`echo $temp`
done
temp=`echo 'scale=6; ('$num')/10' | bc -l`
echo ' & & & avg &' $temp '\\'
echo '\hline'
done

unset num
unset temp
for typ in cpu io mixed
do
echo -ne $sched
echo -ne ' & High & '
echo -ne $typ
echo -ne ' & 1 & '
num=`sudo ./pa3 250 $sched $typ extime`
echo $num '\\'
echo '\cline{4-5}'
for ((i=0; i<9; i++))
do
temp=`sudo ./pa3 250 $sched $typ extime`
echo ' & & &' $((i+2)) '&' $temp '\\'
echo '\cline{4-5}'
num=`echo $num`+`echo $temp`
done
temp=`echo 'scale=6; ('$num')/10' | bc -l`
echo ' & & & avg &' $temp '\\'
echo '\hline'
done

done

echo '\end{tabular}'
echo '\end{center}'
