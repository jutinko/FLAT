#!/bin/bash
FOOTPRINT=2000
TRIAL=10

function profile
{
  rm RTreeTimes.dat FLATTimes.dat
  for data in $(ls test*.bin)
  do
    printf "%s\n" $data >> RTreeTimes.dat
    printf "%s\n" $data >> FLATTimes.dat
    name=$(echo $data | cut -d "." -f 1)
    for i in $(seq 1 $TRIAL)
    do
      ../bin/./RtreeTest --datafile $data --queryfile $name.txt >> RTreeTimes.dat
      ../bin/./FLATGenerator --datafile $data --footprint $FOOTPRINT --queryfile $name.txt >> FLATTimes.dat
    done 
    echo >> RTreeTimes.dat
    echo >> FLATTimes.dat
  done
}

profile
