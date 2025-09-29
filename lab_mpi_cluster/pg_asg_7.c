#include <assert.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define BUFFER_SIZE 256

int main(){
    MPI_Init(NULL, NULL);

    int comm_sz, my_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    assert(comm_sz == 2);

    char message[BUFFER_SIZE];

    /* Clock */
    clock_t start, end;
    double time_taken;

    /* MPI_Wtime */
    double mpi_start, mpi_end;
    double mpi_time_taken;

    mpi_start = MPI_Wtime(); start = clock();
    if (my_rank == 0) {
        snprintf(message, BUFFER_SIZE, "Hello from process %d", my_rank);
        MPI_Send(message, BUFFER_SIZE, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
        printf("Process %d sent message: %s\n", my_rank, message);
    } else {
        MPI_Recv(message, BUFFER_SIZE, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Process %d received message: %s\n", my_rank, message);
        printf("Ping\n");
    }

    mpi_end = MPI_Wtime(); end = clock();

    mpi_time_taken = mpi_end - mpi_start; time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;

    printf("Process %d: Time taken using MPI_Wtime: %f seconds, time taken using clock(): %f seconds\n", my_rank, mpi_time_taken, time_taken);

    MPI_Barrier(MPI_COMM_WORLD);

    mpi_start = MPI_Wtime(); start = clock();

    if (my_rank == 1) {
        if (message[0] != '\0'){
            snprintf(message, BUFFER_SIZE, "Got you from process %d", my_rank);
            MPI_Send(message, BUFFER_SIZE, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
            printf("Process %d sent message: %s\n", my_rank, message);
        }
    } else {
        if (message[0] != '\0'){
            MPI_Recv(message, BUFFER_SIZE, MPI_CHAR, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            printf("Process %d received back message: %s\n", my_rank, message);
            printf("Pong!\n");
        }
    }

    mpi_end = MPI_Wtime(); end = clock();

    mpi_time_taken = mpi_end - mpi_start; time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;

    printf("Process %d: Time taken using MPI_Wtime: %f seconds, time taken using clock(): %f seconds\n", my_rank, mpi_time_taken, time_taken);

    MPI_Finalize();
    return 0;
}