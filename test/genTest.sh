#!/bin/bash

for i in $(seq 1 10)
do
  n=$(($i*100000))
  q=$(($n/1000))
  name="test"$i
  echo $name
  ../bin/./GenerateRandomData --distribution uniform --dataFile test$i"00K"$i"K".bin --queryFile test$i"00K"$i"K".txt --elements $n --queryRatio 0.01
done
