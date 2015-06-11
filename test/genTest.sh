#!/bin/bash
RATIO="0.01"


for i in $(seq 1 1)
do
  n=$(($i*70000000))
  k=$(($n/1000))
  #q=$(($k*$RATIO))
  q=$(echo "$k*$RATIO" | bc | cut -d . -f 1)
  name=test$k"K"$q"K"
  echo $name
  ../bin/./GenerateRandomData --distribution uniform --dataFile $name.bin --queryFile $name.txt --elements $n --queryRatio $RATIO
done
