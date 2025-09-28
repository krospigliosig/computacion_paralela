#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

/*---------------------------------------------------------------------
 * Function:    Monte_carlo
 * Purpose:     Estimate the value of pi using the Monte Carlo method.
 * In args:     number_of_tosses: number of random points to generate
 * Out args:    number_of_hits: number of points that fall inside the unit circle
 */
void Monte_carlo(
    long long number_of_tosses    /* in */,
    long long* number_of_hits     /* out */);

int main(){
    srand(1);

    MPI_Init(NULL, NULL);

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    long long n_tosses = 0;
    long long n_hits = 0;

    if (world_rank == 0) {
        printf("Ingrese el número de lanzamientos: ");
        fflush(stdout);
        scanf("%lld", &n_tosses);
    }

    MPI_Bcast(&n_tosses, 1, MPI_LONG_LONG, 0, MPI_COMM_WORLD);
    
    long long tosses_per_process = n_tosses / world_size;

    if (world_rank == world_size - 1) {
        tosses_per_process += n_tosses % world_size; // last process takes the remainder
    }

    long long local_n_hits = 0;
    Monte_carlo(tosses_per_process, &local_n_hits);
    MPI_Reduce(&local_n_hits, &n_hits, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    double pi_estimate = 4 * (double)n_hits / (double)n_tosses;

    if (world_rank == 0) printf("Number of tosses: %lld\nEstimate of Pi\t%f\n", n_tosses, pi_estimate);

    MPI_Finalize();
    return 0;
}

void Monte_carlo(
        long long number_of_tosses    /* in */,
        long long* number_of_hits     /* out */) {
    
    for (int toss = 0; toss < number_of_tosses; ++toss){
        float x = -1 + ((float)rand() / (float)RAND_MAX) * 2;
        float y = -1 + ((float)rand() / (float)RAND_MAX) * 2;

        float distance_squared = x * x + y * y;
        if (distance_squared <= 1) ++*number_of_hits;
    }
}