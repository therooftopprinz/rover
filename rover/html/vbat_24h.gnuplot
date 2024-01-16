load 'vbat_common.gnuplot'

set output 'vbat_24h.png_'
plot 'vbat_24h.csv' u 1:2 title 'vbus'     w line ls 1, \
     ''             u 1:3 title 'lp(vbus)' w line ls 2