#!/bin/bash

program="APSP_MPI_sync"
p='-p batch'
pc=('1' '2' '3' '4')
c=('800' '800' '800' '800')
testcase=("v800_e800" "v800_e50k" "v800_e100k" "v800_e300k")
root="testcase/"
log="mpi_async_time_measure.log"


if [ -f "$log" ] ; then
    echo -e "$log existed!! -> \e[1;31mremove\e[0m"
    rm $log
fi

for ((n=0;n<${#testcase[@]};++n)); do
    inp="${root}${testcase[$n]}.in"
    ans="${root}${testcase[$n]}.out"
    oup="${testcase[$n]}.out"

    for ((i=0;i<${#c[@]};++i)); do
        if [ -f "$oup" ] ; then
            echo -e "$oup existed!! -> \e[1;31mremove\e[0m"
            rm ${oup}
        fi
        echo "srun $p -N ${pc[$i]} -n ${c[$i]} ./${program} $inp $oup ${c[$i]}" | tee -a $log
        { time srun $p -N ${pc[$i]} -n ${c[$i]} ./${program} $inp $oup ${c[$i]} | tee -a $log ; } | tee -a $log

        diff $oup $ans
        if [ $? == 0 ] ; then
            echo -e "\e[1;32mCorrect\e[0m"
        else
            echo -e "\e[1;31mWrong\e[0m"
        fi
    done
done
