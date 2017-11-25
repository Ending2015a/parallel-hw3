#!/bin/bash

program="APSP_Pthread"
p='-p batch'
c=('1' '2' '4' '8' '16' '32' '48' '64' '80' '96' '112' '128' '2000') 
testcase="testcase/v2k_e200k.in"
answer="testcase/v2k_e200k.out"
output="v2k_e200k.out"

if [ -f "log.txt" ] ; then
    echo -e "log.txt existed!! -> \e[1;31mremove\e[0m"
    rm log.txt
fi

for ((i=0;i<${#c[@]};++i)); do
    if [ -f "$output" ] ; then
        echo -e "$output existed!! -> \e[1;31mremove\e[0m"
        rm ${output}
    fi
    echo "srun $p -c 2000 ./${program} $testcase $output 2000 ${c[$i]}"
    time srun $p -c 2000 ./${program} $testcase $output 2000 ${c[$i]} >> log.txt

    diff $output $answer
    if [ $? == 0 ] ; then
        echo -e "\e[1;32mCorrect\e[0m"
    else
        echo -e "\e[1;31mWrong\e[0m"
    fi
done