#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_VALUE 1000
#define MIN_VALUE 10

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    int local_val = MIN_VALUE + (rand() % (MAX_VALUE - MIN_VALUE));

    int p2 = 1;
    while (p2 * 2 <= world_size) p2 *= 2;

    if (world_rank >= p2) {
        int target = world_rank - p2;
        MPI_Send(&local_val, 1, MPI_INT, target, 0, MPI_COMM_WORLD);
    } else if (world_rank + p2 < world_size) {
        int recv_val;
        MPI_Recv(&recv_val, 1, MPI_INT, world_rank + p2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        local_val += recv_val;
    }

    int gap = 1;
    while (gap < p2) {
        if (world_rank % (2 * gap) == 0) {
            int src = world_rank + gap;
            if (src < p2) {
                int recv_val;
                MPI_Recv(&recv_val, 1, MPI_INT, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                local_val += recv_val;
            }
        } else {
            int target = world_rank - gap;
            MPI_Send(&local_val, 1, MPI_INT, target, 0, MPI_COMM_WORLD);
            break;
        }
        gap *= 2;
    }

    if (world_rank == 0) printf("Total sum: %d\n", local_val);

    MPI_Finalize();
    return 0;
}
