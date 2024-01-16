set datafile separator ','
set xdata time
set timefmt "%s"
set format x "%m%dT%H:%M:%S"
set xtics rotate by -90

set mxtics 2
set mytics 2

set style line  1 lc rgb '#007700' lt 1 lw 1
set style line  2 lc rgb '#770000' lt 1 lw 2
set style line 10 lc rgb '#000000' lt 0 lw 1
set style line 11 lc rgb '#000000' lt 0 lw 0.25

set grid xtics ytics mxtics mytics ls 10, ls 11

set term pngcairo size 700,358 crop
