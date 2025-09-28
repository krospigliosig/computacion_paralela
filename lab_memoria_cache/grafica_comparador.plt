set terminal pngcairo size 800,600 enhanced font "Arial,12"
set output "grafica.png"

set title "Multiplicación de matrices"
set xlabel "Tamaño N"
set ylabel "Tiempo (s)"
set grid
plot "data_clasic.dat" using 1:2 with linespoints title "Algoritmo clásico", \
     "data_blocks.dat" using 1:2 with linespoints title "Algoritmo por bloques"

set output

