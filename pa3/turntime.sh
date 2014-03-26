#!/bin/bash

echo '\begin{center}'
echo '\begin{tabular}{|c|c|c|c|c|}'
echo '\hline'
echo 'Schedule & Utilization & Type & Trial & Turnaround Time\\'
echo '\hline'
echo '\hline'

for sched in SCHED_OTHER SCHED_FIFO SCHED_RR
do
unset num
unset temp
unset fo
for typ in cpu io mixed
do
echo -ne $sched
echo -ne ' & Low & '
echo -ne $typ
echo -ne ' & 1 & ' 
num=`sudo ./pa3 10 $sched $typ turntime`
num=${num%?}
fo=`echo 'scale=6; ('$num')/10' | bc -l`
echo $fo '\\'
echo '\cline{4-5}'
for ((i=0; i<9; i++))
do
temp=`sudo ./pa3 10 $sched $typ turntime`
num=`echo $num`+`echo ${temp%?}`
echo -ne ' & & &' $((i+2)) '& '
fo=`echo 'scale=6; ('${temp%?}')/10' | bc -l`
echo $fo '\\'
echo '\cline{4-5}'
done
echo -ne ' & & & avg & '
fo=`echo 'scale=6; ('${num%?}')/100' | bc -l`
echo $fo '\\'
echo '\hline'
done

unset num
unset temp
unset fo
for typ in cpu io mixed
do
echo -ne $sched
echo -ne ' & Medium & '
echo -ne $typ
echo -ne ' & 1 & ' 
num=`sudo ./pa3 50 $sched $typ turntime`
num=${num%?}
fo=`echo 'scale=6; ('$num')/50' | bc -l`
echo $fo '\\'
echo '\cline{4-5}'
for ((i=0; i<9; i++))
do
temp=`sudo ./pa3 50 $sched $typ turntime`
num=`echo $num`+`echo ${temp%?}`
echo -ne ' & & &' $((i+2)) '& '
fo=`echo 'scale=6; ('${temp%?}')/50' | bc -l`
echo $fo '\\'
echo '\cline{4-5}'
done
echo -ne ' & & & avg & '
fo=`echo 'scale=6; ('${num%?}')/500' | bc -l`
echo $fo '\\'
echo '\hline'
done

unset num
unset temp
unset fo
for typ in cpu io mixed
do
echo -ne $sched
echo -ne ' & High & '
echo -ne $typ
echo -ne ' & 1 & ' 
num=`sudo ./pa3 250 $sched $typ turntime`
num=${num%?}
fo=`echo 'scale=6; ('$num')/250' | bc -l`
echo $fo '\\'
echo '\cline{4-5}'
for ((i=0; i<9; i++))
do
temp=`sudo ./pa3 250 $sched $typ turntime`
num=`echo $num`+`echo ${temp%?}`
echo -ne ' & & &' $((i+2)) '& '
fo=`echo 'scale=6; ('${temp%?}')/250' | bc -l`
echo $fo '\\'
echo '\cline{4-5}'
done
echo -ne ' & & & avg & '
fo=`echo 'scale=6; ('${num%?}')/2500' | bc -l`
echo $fo '\\'
echo '\hline'
done
done

echo '\end{tabular}'
echo '\end{center}'
