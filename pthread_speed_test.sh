#!/bin/bash

program="APSP_Pthread"
p='-p batch'
c=('11' '12' '16' '32' '48' '64' '80' '96' '112' '128' '2000') 
testcase=("v2k_e2k" "v2k_e200k" "v2k_e1m" "v2k_e2m")
root="testcase/"

if [ -f "log.txt" ] ; then
    echo -e "log.txt existed!! -> \e[1;31mremove\e[0m"
    rm log.txt
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
        echo "srun $p -N 1 -c 12 ./${program} $inp $oup ${c[$i]}"
        time srun $p -N 1 -c 12 ./${program} $inp $oup ${c[$i]} | tee -a log.txt

        diff $oup $ans
        if [ $? == 0 ] ; then
            echo -e "\e[1;32mCorrect\e[0m"
        else
            echo -e "\e[1;31mWrong\e[0m"
        fi
    done
done
