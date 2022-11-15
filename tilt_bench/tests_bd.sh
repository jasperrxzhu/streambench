#! /usr/bin/bash

OUTPUT_FILE="tests_1115_v02/bd_output.txt"

for i in {1,2,4,6,8,10,12,16}
do
    STR="Running tests with "
    STR+=$i
    STR+=" threads"
    SIZE=$((120000000/$i))
    echo $SIZE
    echo "$STR" >> $OUTPUT_FILE
    echo "bdselect"
    for j in {1..5}
    do
        ./build/main bdselect $SIZE $i >> $OUTPUT_FILE
    done
    echo "bdwhere"
    for j in {1..5}
    do
        ./build/main bdwhere $SIZE $i >> $OUTPUT_FILE
    done
    echo "bdagg"
    for j in {1..5}
    do
        ./build/main bdagg $SIZE $i >> $OUTPUT_FILE
    done
done