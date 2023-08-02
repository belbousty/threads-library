#!/bin/bash

TESTS="01-main 02-switch 03-equity 11-join 12-join-main 21-create-many 22-create-many-recursive 23-create-many-once 31-switch-many 32-switch-many-join 33-switch-many-cascade 51-fibonacci 52-sum-list 61-mutex 62-mutex 63-mutex-equity 71-preemption 81-deadlock 82-join-same 83-deadlock"
PARAMS=("" "" "" "" "" "100" "20" "100" "5 100" "5 100" "5 100" "10" "10" "20" "20" "" "5" "" "" "")

i=0
for test in ${TESTS}
do
    if [ -f "./install/bin/${test}" ]; then
        echo -n "TEST : "
        echo ${test}
        res=0
        if [ $# -ge 1 ] && [ $1 = "valgrind" ]; then
            output=$(LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./install/lib valgrind --leak-check=full --show-reachable=yes --track-origins=yes ./install/bin/${test} ${PARAMS[$i]} 2>&1);
            echo "$output" > ${test}_valgrind.txt
            if echo "$output" | grep -E "(LEAK SUMMARY)" >/dev/null; then
                res=1
            fi
        else
            LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./install/lib ./install/bin/${test} ${PARAMS[$i]} > ${test}.txt 2>&1;
            res=$?
        fi
        if [ $res -eq 0 ]; then
            echo "Passed"
        else
            echo "Failed !!!"
        fi
    else
        echo -n ${test}
        echo " not found, please compile all files with $ make"
        break
    fi
    i=$((i+1));
done
