#!/bin/bash
RATIO="0.5"


for i in $(seq 1 10)
do
  n=$(($i*100000))
  k=$(($n/1000))
  #q=$(($k*$RATIO))
  q=$(echo "$k*$RATIO" | bc | cut -d . -f 1)
  name=test$k"K"$q"K"
  echo $name
  ../bin/./GenerateRandomData --distribution uniform --dataFile $name.bin --queryFile $name.txt --elements $n --queryRatio $RATIO
done
