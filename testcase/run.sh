#!/bin/bash


program=('ms_seq' 'ms_mpi_static' 'ms_mpi_dynamic' 'ms_omp' 'ms_hybrid')
pic_name=('seq.png' 'stc.png' 'dyn.png' 'omp.png' 'hyb.png')
total_case_gen=10
p='-p batch'
N=('1' '2' '2' '1'  '2')
n=('1' '8' '8' '1'  '4')
c=('1' '1' '1' '12' '4')
height=1
width=1
program_num=${#program[@]}
pass=0
all_pass=1
lower=-3
upper=3



for ((i=0;i<$total_case_gen;++i)); do
    
    low=$(python gen_rand.py $lower $upper)
    up=$(python gen_rand.py $low $upper)
    left=$(python gen_rand.py $lower $upper)
    right=$(python gen_rand.py $left $upper)

    echo "[ for case $i] : left=$left right=$right low=$low up=$up"

    for ((j=0;j<$program_num;++j)); do
        if [ -f "${pic_name[$j]}" ] ; then
            rm ${pic_name[$j]}
        fi
        echo "srun $p -N ${N[$j]} -n ${n[$j]} -c ${c[$j]} ./${program[$j]} ${c[$j]} $left $right $low $up $width $height ${pic_name[$j]}"
        time srun $p -N ${N[$j]} -n ${n[$j]} -c ${c[$j]} ./${program[$j]} ${c[$j]} $left $right $low $up $width $height ${pic_name[$j]}
    done

    echo "case done -> verifying..."

    case_pass=1
    for ((j=1;j<$program_num;++j)); do
        ans=${pic_name[0]}
        result=$(hw2-diff $ans ${pic_name[$j]})
        if echo "$result" | grep "100.00%" ; then
            echo -e "program ${program[$j]}: [ pass ]"
        else
            echo -e "program ${program[$j]}: [failed]"
            let all_pass=0
            let case_pass=0
        fi
    done
    if [ "${case_pass}" == "1" ] ; then
        let pass="$pass + 1"
        echo -e "case $i: [ pass ]"
    else
        echo -e "case $i: [failed]"
    fi
done

echo "pass $pass/$total_case_gen"
if [ "${all_pass}" == "1" ] ; then
    echo -e "[ ALL PASS ] "
fi
