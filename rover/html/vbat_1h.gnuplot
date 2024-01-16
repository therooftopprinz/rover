load 'vbat_common.gnuplot'

set output 'vbat_1h.png_'
plot 'vbat_1h.csv' u 1:2 title 'vbus'     w line ls 1, \
     ''            u 1:3 title 'lp(vbus)' w line ls 2