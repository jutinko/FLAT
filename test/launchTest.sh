#!/bin/bash
FOOTPRINT=2000
TRIAL=10

function profile
{
  rm RTreeTimes.dat FLATTimes.dat
  for data in $(ls -v test*.bin)
  do
    name=$(echo $data | cut -d "." -f 1)
    printf "%s\n" $name >> RTreeTimes.dat
    printf "%s\n" $name >> FLATTimes.dat
    for i in $(seq 1 $TRIAL)
    do
      ../bin/./RtreeTest --datafile $data --queryfile $name.txt >> RTreeTimes.dat
      ../bin/./FLATGenerator --datafile $data --footprint $FOOTPRINT --queryfile $name.txt >> FLATTimes.dat
      valgrind --tool=callgrind --callgrind-out-file=rtree.$name.callgrind.out ../bin/./RtreeTest --datafile $data --queryfile $data.txt
      valgrind --tool=callgrind --callgrind-out-file=flat.$name.callgrind.out ../bin/./FLATGenerator --datafile $data --queryfile $data.txt
    done 
    echo >> RTreeTimes.dat
    echo >> FLATTimes.dat
  done
}

profile
