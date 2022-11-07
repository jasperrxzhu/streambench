#! /usr/bin/bash

OUTPUT_FILE="tests_1107/float_output.txt"

for i in {1,2,4,6,8,10,12,16,20,24,30,32}
do
    STR="Running tests with "
    STR+=$i
    STR+=" threads"
    SIZE=$((120000000/$i))
    echo $SIZE
    echo "$STR" >> $OUTPUT_FILE
    echo "select"
    for j in {1..5}
    do
        ./build/main select $SIZE $i >> $OUTPUT_FILE
    done
    echo "where"
    for j in {1..5}
    do
        ./build/main where $SIZE $i >> $OUTPUT_FILE
    done
    echo "aggregate"
    for j in {1..5}
    do
        ./build/main aggregate $SIZE $i >> $OUTPUT_FILE
    done
done