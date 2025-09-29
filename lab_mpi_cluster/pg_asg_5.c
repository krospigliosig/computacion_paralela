#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MIN_VALUE 1
#define MAX_VALUE 100

void Gen_mat_vec(int* A, int* x, int n){
    for (int i = 0; i < n; i++) {
        x[i] = MIN_VALUE + (i % (MAX_VALUE - MIN_VALUE));
        for (int j = 0; j < n; j++) {
            A[i * n + j] = MIN_VALUE + ((i + j) % (MAX_VALUE - MIN_VALUE));
        }
    }
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int comm_sz, my_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    srand((unsigned)time(NULL));

    int n = 6;
    if (my_rank == 0) printf("Ejecutando con %d procesos\n", comm_sz);

    if (n % comm_sz != 0) {
        if (my_rank == 0) printf("Tamaño de la matriz %d no es divisible entre número de procesos %d.\n", n, comm_sz);
        MPI_Finalize();
        return 0;
    }

    int cols_per_proc = n / comm_sz;

    int* A = malloc(n * n * sizeof(int));
    int* x = malloc(n * sizeof(int));
    if (my_rank == 0) Gen_mat_vec(A, x, n);

    int* local_A = malloc(n * cols_per_proc * sizeof(int));
    MPI_Datatype column_block;
    MPI_Type_vector(n, cols_per_proc, n, MPI_INT, &column_block);
    MPI_Type_commit(&column_block);

    if (my_rank == 0)
        for (int p = 0; p < comm_sz; p++)
            if (p == 0)
                for (int i = 0; i < n; i++)
                    for (int j = 0; j < cols_per_proc; j++)
                        local_A[i*cols_per_proc + j] = A[i*n + j];
            else MPI_Send(&A[p*cols_per_proc], 1, column_block, p, 0, MPI_COMM_WORLD);
    else MPI_Recv(local_A, n * cols_per_proc, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    int* local_x = malloc(cols_per_proc * sizeof(int));
    if (my_rank == 0)
        for (int p = 0; p < comm_sz; p++)
            if (p == 0)
                for (int j = 0; j < cols_per_proc; j++)
                    local_x[j] = x[j];
            else MPI_Send(&x[p*cols_per_proc], cols_per_proc, MPI_INT, p, 0, MPI_COMM_WORLD);
    else MPI_Recv(local_x, cols_per_proc, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    int* local_partial = calloc(n, sizeof(int));
    for (int i = 0; i < n; i++)
        for (int j = 0; j < cols_per_proc; j++)
            local_partial[i] += local_A[i*cols_per_proc + j] * local_x[j];

    int recv_counts[comm_sz];
    for (int p = 0; p < comm_sz; p++) recv_counts[p] = n / comm_sz;

    int* local_y = malloc(n/comm_sz * sizeof(int));
    MPI_Reduce_scatter(local_partial, local_y, recv_counts, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

    printf("Proceso %d: y_local = ", my_rank);
    for (int i = 0; i < n/comm_sz; i++)
        printf("%d ", local_y[i]);
    printf("\n");

    free(local_A);
    free(local_x);
    free(local_partial);
    free(local_y);
    
    if (my_rank == 0) {
        free(A);
        free(x);
    }

    MPI_Type_free(&column_block);
    MPI_Finalize();
    return 0;
}
