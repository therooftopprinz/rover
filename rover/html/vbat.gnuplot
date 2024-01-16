load 'vbat_common.gnuplot'

set output 'vbat.svg'
plot 'vbat.csv' u 1:2 title 'vbus'     w line ls 1, \
     ''         u 1:3 title 'lp(vbus)' w line ls 2
