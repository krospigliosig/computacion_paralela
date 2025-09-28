set terminal pngcairo size 800,600 enhanced font "Arial,12"
set output "grafica_blocks.png"

set title "Multiplicación de matrices por bloques"
set xlabel "Tamaño N"
set ylabel "Tiempo (s)"
set grid
plot "data_blocks.dat" using 1:2 with linespoints title "Algoritmo por bloques"

set output