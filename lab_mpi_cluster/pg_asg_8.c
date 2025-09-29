#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MIN_VALUE 1
#define MAX_VALUE 100

int cmpfunc(const void* a, const void* b) {
    float fa = *(const float*)a;
    float fb = *(const float*)b;
    if (fa < fb) return -1;
    if (fa > fb) return 1;
    return 0;
}

void Gen_data(float* data, int n) {
    for (int i = 0; i < n; i++)
        data[i] = MIN_VALUE + (MAX_VALUE - MIN_VALUE) * ((float)rand() / (float)RAND_MAX);
}

float* merge_arrays(float* a, int n_a, float* b, int n_b) {
    float* merged = malloc((n_a + n_b) * sizeof(float));

    // err
    if (!merged){
        perror("malloc");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    int i = 0, j = 0, k = 0;
    while (i < n_a && j < n_b) {
        if (a[i] < b[j]) merged[k++] = a[i++];
        else merged[k++] = b[j++];
    }

    while (i < n_a) merged[k++] = a[i++];
    while (j < n_b) merged[k++] = b[j++];
    return merged;
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int comm_sz, my_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    int n;
    if (my_rank == 0) n = 64;
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // err
    if (n % comm_sz != 0) {
        if (my_rank == 0) fprintf(stderr, "Error: n (%d) no divisible por comm_sz (%d)\n", n, comm_sz);
        MPI_Finalize();
        return 1;
    }

    int local_n = n / comm_sz;

    srand((unsigned int)time(NULL) + my_rank * 101);

    float* local_data = malloc(local_n * sizeof(float));

    // err
    if (!local_data) {
        perror("malloc local_data");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    Gen_data(local_data, local_n);

    qsort(local_data, local_n, sizeof(float), cmpfunc);

    int step = 1;
    while (step < comm_sz) {
        if (my_rank % (2 * step) == 0 && my_rank + step < comm_sz) {
            int recv_n;

            MPI_Recv(&recv_n, 1, MPI_INT, my_rank + step, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            float* recv_data = malloc(recv_n * sizeof(float));

            // err
            if (!recv_data) {
                perror("malloc recv_data");
                MPI_Abort(MPI_COMM_WORLD, 1);
            }

            MPI_Recv(recv_data, recv_n, MPI_FLOAT, my_rank + step, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            float* merged = merge_arrays(local_data, local_n, recv_data, recv_n);

            free(local_data);
            free(recv_data);

            local_data = merged;
            local_n += recv_n;
        } else {
            int dest = my_rank - step;

            MPI_Send(&local_n, 1, MPI_INT, dest, 0, MPI_COMM_WORLD);
            MPI_Send(local_data, local_n, MPI_FLOAT, dest, 0, MPI_COMM_WORLD);

            free(local_data);
            break;
        }
        step *= 2;
    }

    if (my_rank == 0) {
        printf("Lista global ordenada (proceso 0):\n");
        for (int i = 0; i < local_n; i++) printf("%0.3f ", local_data[i]);
        printf("\n");
        free(local_data);
    }

    MPI_Finalize();
    return 0;
}
