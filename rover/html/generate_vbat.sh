#!/bin/bash

cd "$(dirname "$0")";

cx=0

while :
do

  if [[ 0 == $(expr $cx % 300) ]]
  then
    echo "u24h"
    tail -86400 vbat.csv > vbat_24h.csv 
    gnuplot vbat_24h.gnuplot
    mv vbat_24h.png_ vbat_24h.png
  fi

  if [[ 0 == $(expr $cx % 5) ]]
  then
    echo "u1h"
    tail -3600  vbat.csv > vbat_1h.csv
    gnuplot vbat_1h.gnuplot
    mv vbat_1h.png_ vbat_1h.png
  fi

  tail -300   vbat.csv > vbat_5m.csv

  gnuplot vbat_5m.gnuplot
  mv vbat_5m.png_ vbat_5m.png
  sleep 1
  cx=$(expr $cx + 1)
  cx=$(expr $cx % 3600)
done
