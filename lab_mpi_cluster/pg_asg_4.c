#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_VALUE 1000
#define MIN_VALUE 10

int main(int argc, char** argv) {
    MPI_Init(NULL, NULL);

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    int local_val = MIN_VALUE + (rand() % (MAX_VALUE - MIN_VALUE));

    printf("Proceso %d tiene valor %d\n", world_rank, local_val);

    int sum = local_val;

    int mask = 1;
    while (mask < world_size) {
        int partner = world_rank ^ mask;

        if (partner < world_size) {
            int received_val;
            MPI_Sendrecv(&sum, 1, MPI_INT, partner, 0, &received_val, 1, MPI_INT, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            sum += received_val;
        }

        mask <<= 1;
    }

    printf("Proceso %d suma total = %d\n", world_rank, sum);

    MPI_Finalize();
    return 0;
}
