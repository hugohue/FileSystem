#!/bin/bash
source utils/ds_cases.sh

red='\033[0;31m'
green='\033[0;32m'
cyan='\033[0;36m'
nc='\033[0m'

BD="\033[1m"
BDED="\033[21m"
RED="\033[31m"
GREEN="\033[92m"
YELLOW="\033[93m"

pass=0
total=0

function judge {
    sudo umount mnt &> /dev/null
    printf "Testcase %s %s: " "$1" "$2"
    echo -ne "${cyan}WAITING${nc}"
    ./cases/$1/$2
    if [ $? -eq 0 ]; then
        echo -e "\b\b\b\b\b\b\b${green}PASSED ${nc}"
        (( pass ++ ));
    else
        #echo -e "\b\b\b\b\b\b\b${red}FAILED ${nc}"
        echo -e "${red}FAILED ${nc}"
        i=`echo "$2" | sed "s/c//"`
        if [ $1 == "m1" ]; then
            i=`expr $i - 1`
        elif [ $1 == "m2" ]; then
            i=`expr $i - 1 + 11`
        elif [ $1 == "m3" ]; then
            i=`expr $i - 1 + 11 + 7`
        elif [ $1 == "m4" ]; then
            i=`expr $i - 1 + 11 + 7 + 16`
        fi
           
        echo -e "${red}${ds[$i]}${nc}"
    fi
    ((total ++));
}



if [ $# -eq 2 ]; then
    judge $1 $2
elif [ $# -eq 1 ]; then
    num_cases=`ls cases/$1/c* | wc -l`
    for c in $(seq 1 $num_cases)
    do
        judge $1 c$c
        rm -f disks/*
    done
else
    num_cases=`ls cases/m1/c* | wc -l`
    for c in $(seq 1 $num_cases)
    do
        judge m1 c$c
        rm -f disks/*
    done
    num_cases=`ls cases/m2/c* | wc -l`
    for c in $(seq 1 $num_cases)
    do
        judge m2 c$c
        rm -f disks/*
    done
    num_cases=`ls cases/m3/c* | wc -l`
    for c in $(seq 1 $num_cases)
    do
        judge m3 c$c
        rm -f disks/*
    done
    num_cases=`ls cases/m4/c* | wc -l`
    for c in $(seq 1 $num_cases)
    do
        judge m4 c$c
        rm -f disks/*
    done
fi

echo -e "${GREEN}""[Result] ${pass}/${total} test cases passed${ED}"

if [ $((pass)) -eq $((total)) ]; then
    echo -e "${GREEN}Congrats!${ED}"
fi
