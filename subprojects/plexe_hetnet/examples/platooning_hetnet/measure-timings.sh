#!/usr/bin/env bash

runs="0 1 2"
config="ScalabilityNoGui"
file="scalability-timings.txt"

rm -f $file

for i in $runs;
do
    echo $i >> $file
    { time ./run -u Cmdenv -c $config -r $i 2>1 ; } 2>> $file
done
