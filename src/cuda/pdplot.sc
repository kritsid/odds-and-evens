set terminal png
set output "resultDfDx.png"
set pm3d map
unset xtics
unset ytics
set size square
set title "df/dx" offset 0,2
splot "resultDfDx.txt" u 1:2:3

set output "resultDfDy.png"
set pm3d map
unset xtics
unset ytics
set size square
set title "df/dy" offset 0,2
splot "resultDfDy.txt" u 1:2:3
