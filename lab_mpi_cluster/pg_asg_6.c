#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MIN_VALUE 1
#define MAX_VALUE 100

void Gen_matrix_vector(int* A, int* x, int n) {
    for (int i = 0; i < n; i++) {
        x[i] = MIN_VALUE + (rand() % (MAX_VALUE - MIN_VALUE));
        for (int j = 0; j < n; j++) A[i*n + j] = MIN_VALUE + (rand() % (MAX_VALUE - MIN_VALUE));
    }
}

void Print_matrix(int* A, int n) {
    printf("Matriz A:\n");
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++)
            printf("%3d ", A[i*n + j]);
        printf("\n");
    }
}

void Print_vector(int* v, int n, char* name) {
    printf("%s: ", name);
    for (int i = 0; i < n; i++) printf("%d ", v[i]);
    printf("\n");
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int comm_sz, my_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    int q = (int) sqrt(comm_sz);
    if (q * q != comm_sz) {
        if (my_rank == 0) printf("Error: comm_sz debe ser un cuadrado perfecto.\n");
        MPI_Finalize();
        return 0;
    }

    int n = 4;
    if (n % q != 0) {
        if (my_rank == 0) printf("Error: n debe ser divisible entre sqrt(comm_sz).\n");
        MPI_Finalize();
        return 0;
    }

    int block_size = n / q;

    int dims[2] = {q, q};
    int periods[2] = {0, 0};
    MPI_Comm grid_comm;
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 1, &grid_comm);

    int coords[2];
    MPI_Cart_coords(grid_comm, my_rank, 2, coords);
    int my_row = coords[0], my_col = coords[1];

    int *A = NULL, *x = NULL;
    if (my_rank == 0) {
        A = malloc(n * n * sizeof(int));
        x = malloc(n * sizeof(int));
        Gen_matrix_vector(A, x, n);
        Print_matrix(A, n);
        Print_vector(x, n, "Vector x");
    }

    int* local_A = malloc(block_size * block_size * sizeof(int));

    if (my_rank == 0)
        for (int p = 0; p < comm_sz; p++) {
            int c[2];
            MPI_Cart_coords(grid_comm, p, 2, c);
            int row_offset = c[0] * block_size;
            int col_offset = c[1] * block_size;

            int* temp_block = malloc(block_size * block_size * sizeof(int));
            for (int i = 0; i < block_size; i++)
                for (int j = 0; j < block_size; j++)
                    temp_block[i*block_size + j] = A[(row_offset+i)*n + (col_offset+j)];

            if (p == 0)
                for (int i = 0; i < block_size*block_size; i++)
                    local_A[i] = temp_block[i];
            else MPI_Send(temp_block, block_size*block_size, MPI_INT, p, 0, MPI_COMM_WORLD);
            
            free(temp_block);
        }
    else MPI_Recv(local_A, block_size*block_size, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    int* local_x = malloc(block_size * sizeof(int));
    if (my_row == my_col)
        if (my_rank == 0) {
            for (int j = 0; j < block_size; j++) local_x[j] = x[j];
            for (int p = 1; p < q; p++)
                MPI_Send(&x[p*block_size], block_size, MPI_INT, p*(q+1), 0, MPI_COMM_WORLD);
        } else MPI_Recv(local_x, block_size, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    MPI_Comm col_comm;
    MPI_Comm_split(grid_comm, my_col, my_row, &col_comm);
    if (my_row == my_col)
        MPI_Bcast(local_x, block_size, MPI_INT, my_row, col_comm);
    else
        MPI_Bcast(local_x, block_size, MPI_INT, my_col, col_comm);

    int* local_y = calloc(block_size, sizeof(int));
    for (int i = 0; i < block_size; i++)
        for (int j = 0; j < block_size; j++)
            local_y[i] += local_A[i*block_size + j] * local_x[j];

    int* reduced_y = NULL;
    if (my_col == 0) reduced_y = malloc(block_size * sizeof(int));

    MPI_Comm row_comm;
    MPI_Comm_split(grid_comm, my_row, my_col, &row_comm);
    MPI_Reduce(local_y, reduced_y, block_size, MPI_INT, MPI_SUM, 0, row_comm);

    if (my_col == 0) {
        printf("Proceso %d (fila %d): y_local = ", my_rank, my_row);
        for (int i = 0; i < block_size; i++) printf("%d ", reduced_y[i]);
        printf("\n");
    }

    free(local_A);
    free(local_x);
    free(local_y);
    if (my_col == 0) free(reduced_y);
    if (my_rank == 0) { free(A); free(x); }

    MPI_Comm_free(&grid_comm);
    MPI_Comm_free(&row_comm);
    MPI_Comm_free(&col_comm);

    MPI_Finalize();
    return 0;
}
