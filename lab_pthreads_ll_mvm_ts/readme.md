# Laboratorio Pthreads

## Lista enlazada multithreading

Pacheco, P. (2011)[^1] propone una implementación de lista enlazada usando read-write locks para controlar el acceso a la lista según la operación que desee realizar el thread (p. 187). El lock se bloquea previo a la llamada a cualquier función, desbloqueándose inmediatamente después.
Dado que la llamada a las funciones de bloqueo y desbloqueo del lock se realizan dentro de la función `worker`, se decidió, con fines comparativos, realizar una implementación que llame a las funciones de bloqueo y desbloqueo de forma conveniente dentro de los métodos de la lista enlazada y comparar el rendimiento con respecto al tiempo.

### Función `worker` de elaboración propia

```C
void* worker(void* arg){
    int id = *(int*)arg;
    unsigned int seed = (unsigned int)time(NULL) ^ (unsigned int)(id * 7919);

    for (int i = 0; i < total_ops; ++i){
        int v = rand_r(&seed) % 10000;
        double r = (double)rand_r(&seed) / RAND_MAX;

        if (r < ratios.insert_pct)
            Insert(v);
        else if (r < ratios.insert_pct + ratios.search_pct)
            Search(v);
        else
            Delete(v);
    }

    return NULL;
}
```

### Resultados de la comparación

#### Número de threads: 8

Porcentaje de operaciones (`Insert`, `Search`, `Delete`): 0.05, 0.9, 0.05

| Número de operaciones | Tiempo de la implementación original (en segundos) | Tiempo de la implementación propuesta (en segundos) |
|:----:|:----:|:----:|
| 1000 | 2.258062e-03 | 8.688170e-02 |
| 5000 | 4.313183e-02 | 6.331930e-01 |
| 10000 | 9.896183e-02 | 1.298074e+00 |

Porcentaje de operaciones (`Insert`, `Search`, `Delete`): 0.1, 0.8, 0.1

| Número de operaciones | Tiempo de la implementación original (en segundos) | Tiempo de la implementación propuesta (en segundos) |
|:----:|:----:|:----:|
| 1000 | 1.398087e-03 | 9.700900e-02 |
| 5000 | 3.159690e-02 | 6.632232e-01 |
| 10000 | 1.228471e-01 | 1.446373e+00 |

Porcentaje de operaciones (`Insert`, `Search`, `Delete`): 0.2, 0.7, 0.1

| Número de operaciones | Tiempo de la implementación original (en segundos) | Tiempo de la implementación propuesta (en segundos) |
|:----:|:----:|:----:|
| 1000 | 1.331091e-03 | 1.102069e-01 |
| 5000 | 5.698109e-02 | 8.504495e-01 |
| 10000 | 1.460679e-01 | 2.197287e+00 |

Como se puede apreciar, la implementación original propuesta por Pacheco, P. (2011)[^1] demuestra mejores resultados en cuanto al tiempo. Esto es porque, al ser la función `worker` la encargada de bloquear/desbloquear el read-write lock, el thread en turno tendrá posesión del lock mientras lo necesite; cosa que no pasa si es el método el que controla el acceso al lock y cada thread solo puede acceder mientras llame a esta función. 

## Producto matriz-vector con threads

Pacheco, P. (2011)[^1] luego propone una implementación del producto matriz-vector utilizando threads para paralelizar la multiplicación (pp. 159-161). En esta, la asignación de variables se hace dentro de la función `worker`, es decir, cada thread hace el cálculo de la porción de la matriz y del vector que deberá multiplicar. Con motivos de comparación, se realizó una implementación en la que sea el programa principal quien realice este cálculo y entregue a la función `worker` estos valores a través de una struct `ThreadData`.

### Struct `ThreadData`

```C
typedef struct {
    int my_rank;
    int start_row;
    int end_row;
} ThreadData;
```

### Resultados de la comparación

Número de threads: 4

| Número de filas y columnas (filas, columnas) | Tiempo promedio de la implementación original (en segundos) | Tiempo promedio de la implementación propuesta (en segundos) |
|:----:|:----:|:----:|
| (256, 192) | 4.30e-05 | 6.64e-05 |
| (767, 480) | 4.31e-04 | 4.31e-04 |

Número de threads: 8

| Número de filas y columnas (filas, columnas) | Tiempo promedio de la implementación original (en segundos) | Tiempo promedio de la implementación propuesta (en segundos) |
|:----:|:----:|:----:|
| (256, 192) | 2.87e-05 | 3.48e-05 |
| (767, 480) | 2.38e-04 | 3.11e-04 |

Número de threads: 16

| Número de filas y columnas (filas, columnas) | Tiempo promedio de la implementación original (en segundos) | Tiempo promedio de la implementación propuesta (en segundos) |
|:----:|:----:|:----:|
| (256, 192) | 1.59e-05 | 1.88e-05 |
| (767, 480) | 1.41e-04 | 1.22e-04 |

Se puede interpretar de los resultados obtenidos que la cantidad de threads impacta directamente en el rendimiento de la ejecución. Más llamativo resulta el descenso marcado en el tiempo de ejecución de la función implementada a medida que este número aumenta, llegando a ser incluso menor al de la implementación original en el último caso de prueba; lo que podría sugerir que, de asignarse más threads, la tendencia podría confirmarse. Tras realizar una última evaluación con 32 threads, los resultados obtenidos son los siguientes:

| Número de filas y columnas (filas, columnas) | Tiempo promedio de la implementación original (en segundos) | Tiempo promedio de la implementación propuesta (en segundos) |
|:----:|:----:|:----:|
| (256, 192) | 8.36e-06 | 9.77e-06 |
| (767, 480) | 5.23e-05 | 5.93e-05 |

Como se puede apreciar, una última evaluación refleja que la tendencia es real, pero no garantiza menores resultados que los obtenidos con la implementación original.

[^1]: Pacheco, P. S. (2011). Chapter 4 - Shared-Memory Programming with Pthreads. In P. S. Pacheco (Ed.), *An Introduction to Parallel Programming* (pp. 151–207). doi:10.1016/B978-0-12-374260-5.00004-X