#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

void die(const char *msg) {
    perror(msg);
    exit(1);
}

/** Calcula el layout de bloques para n elementos y p procesos
 *  @param n cantidad total de elementos
 *  @param p cantidad de procesos
 *  @param counts arreglo de tamaño p que contendrá la cantidad de elementos por proceso
 *  @param displs arreglo de tamaño p que contendrá los desplazamientos (offsets) por proceso
*/
void compute_block_layout(int n, int p, int *counts, int *displs) {
    int base = n / p;
    int rem = n % p;
    int off = 0;

    for (int i = 0; i < p; i++) {
        counts[i] = base + (i < rem ? 1 : 0);
        displs[i] = off;
        off += counts[i];
    }
}

/** Construye un buffer (sendbuf) agrupando elementos por destino
 *  @param local_values arreglo con los valores que tengo localmente (size local_n)
 *  @param get_global función que devuelve index global del elemento local j
*/
void build_alltoallv_buffers_from_local_indices(
    int*    sendcounts,
    int*    senddispls,
    int**   sendbuf_ptr,
    int*    local_values,
    int     local_n,
    int     comm_sz,
    int     (*global_of)(int)) {

    /* contar */
    for (int i = 0; i < comm_sz; i++) sendcounts[i] = 0;

    for (int j = 0; j < local_n; j++) {
        int g = global_of(j);
        int dest = g % comm_sz;
        sendcounts[dest]++;
    }

    /* desplazamientos y buffer */
    senddispls[0] = 0;
    for (int i = 1; i < comm_sz; i++)
        senddispls[i] = senddispls[i-1] + sendcounts[i-1];

    int total = senddispls[comm_sz-1] + sendcounts[comm_sz-1];
    int *sendbuf = malloc(total * sizeof(int));

    // err
    if (!sendbuf) die("malloc sendbuf");

    int *cursor = malloc(comm_sz * sizeof(int));

    // err
    if (!cursor) die("malloc cursor");

    /* rellenar */
    for (int i = 0; i < comm_sz; i++) cursor[i] = senddispls[i];

    for (int j = 0; j < local_n; j++) {
        int g = global_of(j);
        int dest = g % comm_sz;
        sendbuf[cursor[dest]++] = local_values[j];
    }

    free(cursor);
    *sendbuf_ptr = sendbuf;
}

int block_owner(int global_index, int n, int comm_sz) {
    int base = n / comm_sz;
    int rem = n % comm_sz;

    int limit = (base + 1) * rem;
    if (global_index < limit)
        return global_index / (base + 1);

    return rem + (global_index - limit) / base;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    
    int comm_sz, my_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    int n = 100000;
    int trials = 20;

    if (my_rank == 0) printf("n=%d\tnro de procesos=%d\tintentos=%d\n", n, comm_sz, trials);

    /* Bloques */
    int *block_counts = malloc(comm_sz * sizeof(int));
    int *block_displs = malloc(comm_sz * sizeof(int));

    compute_block_layout(n, comm_sz, block_counts, block_displs);

    int local_n = block_counts[my_rank];
    int global_start = block_displs[my_rank];

    int *local_block = malloc(local_n * sizeof(int));

    // err
    if (!local_block) die("malloc local_block");

    for (int i = 0; i < local_n; i++)
        local_block[i] = global_start + i;

    int *sendcounts = malloc(comm_sz * sizeof(int));
    int *senddispls = malloc(comm_sz * sizeof(int));

    int *recvcounts = malloc(comm_sz * sizeof(int));
    int *recvdispls = malloc(comm_sz * sizeof(int));

    int (*global_of_block)(int) = NULL;

    for (int i = 0; i < comm_sz; i++) {
        int cnt = n / comm_sz + (i < (n % comm_sz) ? 1 : 0);
        recvcounts[i] = n / comm_sz + (i < (n % comm_sz) ? 1 : 0);
    }

    for (int i = 0; i < comm_sz; i++) sendcounts[i] = 0;

    for (int j = 0; j < local_n; j++) {
        int g = global_start + j;
        int dest = g % comm_sz;
        sendcounts[dest]++;
    }
    
    senddispls[0] = 0;
    for (int i = 1; i < comm_sz; i++)
        senddispls[i] = senddispls[i-1] + sendcounts[i-1];

    int send_total = (comm_sz>0) ? (senddispls[comm_sz-1] + sendcounts[comm_sz-1]) : 0;
    int *sendbuf = malloc(send_total * sizeof(int));

    // err
    if (!sendbuf) die("malloc sendbuf");

    int *cursor = malloc(comm_sz * sizeof(int));

    for (int i = 0; i < comm_sz; i++) cursor[i] = senddispls[i];

    for (int j = 0; j < local_n; j++) {
        int g = global_start + j;
        int dest = g % comm_sz;
        sendbuf[cursor[dest]++] = local_block[j];
    }

    free(cursor);

    MPI_Alltoall(sendcounts, 1, MPI_INT, recvcounts, 1, MPI_INT, MPI_COMM_WORLD);

    recvdispls[0] = 0;

    for (int i = 1; i < comm_sz; i++)
        recvdispls[i] = recvdispls[i-1] + recvcounts[i-1];

    int recv_total = (comm_sz>0) ? (recvdispls[comm_sz-1] + recvcounts[comm_sz-1]) : 0;
    int *recvbuf = malloc(recv_total * sizeof(int));

    // err
    if (!recvbuf) die("malloc recvbuf");

    // wait
    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Alltoallv(sendbuf, sendcounts, senddispls, MPI_INT, recvbuf, recvcounts, recvdispls, MPI_INT, MPI_COMM_WORLD);

    /* Medición de tiempo promedio de Blocks -> Cyclic */    
    double *times = malloc(trials * sizeof(double));
    for (int t = 0; t < trials; t++) {
        // wait
        MPI_Barrier(MPI_COMM_WORLD);

        double t0 = MPI_Wtime();
        MPI_Alltoallv(sendbuf, sendcounts, senddispls, MPI_INT, recvbuf, recvcounts, recvdispls, MPI_INT, MPI_COMM_WORLD);

        // wait
        MPI_Barrier(MPI_COMM_WORLD);

        double t1 = MPI_Wtime();
        double elapsed = t1 - t0;
        double max_elapsed;

        MPI_Reduce(&elapsed, &max_elapsed, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
        if (my_rank == 0) times[t] = max_elapsed;
    }

    if (my_rank == 0) {
        double sum = 0;
        for (int t = 0; t < trials; t++) sum += times[t];
        printf("Block -> Cyclic avg (max de los procesos) = %.6e s\n", sum / trials);
    }

    /* Ciclos */
    int cyclic_local_n = recv_total;
    int *cyclic_data = malloc(cyclic_local_n * sizeof(int));

    for (int i = 0; i < cyclic_local_n; i++)
        cyclic_data[i] = recvbuf[i];

    int *sendcounts2 = malloc(comm_sz * sizeof(int));
    int *senddispls2 = malloc(comm_sz * sizeof(int));

    int *recvcounts2 = malloc(comm_sz * sizeof(int));
    int *recvdispls2 = malloc(comm_sz * sizeof(int));

    for (int i = 0; i < comm_sz; i++) sendcounts2[i] = 0;

    for (int j = 0; j < cyclic_local_n; j++) {
        int g = cyclic_data[j];
        int dest = block_owner(g, n, comm_sz);
        sendcounts2[dest]++;
    }

    senddispls2[0] = 0;
    for (int i = 1; i < comm_sz; i++)
        senddispls2[i] = senddispls2[i-1] + sendcounts2[i-1];

    int stotal2 = senddispls2[comm_sz-1] + sendcounts2[comm_sz-1];
    int *sendbuf2 = malloc(stotal2 * sizeof(int));

    // err
    if (!sendbuf2) die("malloc sendbuf2");
    
    cursor = malloc(comm_sz * sizeof(int));

    for (int i = 0; i < comm_sz; i++) cursor[i] = senddispls2[i];

    for (int j = 0; j < cyclic_local_n; j++) {
        int g = cyclic_data[j];
        int dest = block_owner(g, n, comm_sz);
        sendbuf2[cursor[dest]++] = g;
    }
    free(cursor);

    MPI_Alltoall(sendcounts2, 1, MPI_INT, recvcounts2, 1, MPI_INT, MPI_COMM_WORLD);

    recvdispls2[0] = 0;
    for (int i = 1; i < comm_sz; i++)
        recvdispls2[i] = recvdispls2[i-1] + recvcounts2[i-1];

    int rtotal2 = recvdispls2[comm_sz-1] + recvcounts2[comm_sz-1];
    int *recvbuf2 = malloc(rtotal2 * sizeof(int));

    // err
    if (!recvbuf2) die("malloc recvbuf2");

    // wait
    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Alltoallv(sendbuf2, sendcounts2, senddispls2, MPI_INT, recvbuf2, recvcounts2, recvdispls2, MPI_INT, MPI_COMM_WORLD);

    /* Medición de tiempo promedio de Cyclic -> Blocks */
    for (int t = 0; t < trials; t++) {
        // wait
        MPI_Barrier(MPI_COMM_WORLD);

        double t0 = MPI_Wtime();

        MPI_Alltoallv(sendbuf2, sendcounts2, senddispls2, MPI_INT, recvbuf2, recvcounts2, recvdispls2, MPI_INT, MPI_COMM_WORLD);
        
        // wait
        MPI_Barrier(MPI_COMM_WORLD);

        double t1 = MPI_Wtime();
        double elapsed = t1 - t0;
        double max_elapsed;

        MPI_Reduce(&elapsed, &max_elapsed, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
        if (my_rank == 0) times[t] = max_elapsed;
    }

    if (my_rank == 0) {
        double sum = 0;

        for (int t = 0; t < trials; t++)
            sum += times[t];
        
        printf("Cyclic -> Block avg (max de los procesos) = %.6e s\n", sum / trials);
    }

    int *gather_counts = NULL;
    int *gather_displs = NULL;

    if (my_rank == 0) gather_counts = malloc(comm_sz * sizeof(int));

    /* Recopilar y revisar que todos los elementos estén presentes  */
    MPI_Gather(&rtotal2, 1, MPI_INT, gather_counts, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (my_rank == 0) {
        gather_displs = malloc(comm_sz * sizeof(int));
        gather_displs[0] = 0;

        for (int i = 1; i < comm_sz; i++)
            gather_displs[i] = gather_displs[i-1] + gather_counts[i-1];

        int total = gather_displs[comm_sz-1] + gather_counts[comm_sz-1];
        int *final = malloc(total * sizeof(int));

        MPI_Gatherv(recvbuf2, rtotal2, MPI_INT, final, gather_counts, gather_displs, MPI_INT, 0, MPI_COMM_WORLD);

        char *seen = calloc(n, 1);
        short ok = 1;

        for (int i = 0; i < total; i++) {
            int v = final[i];
            if (v < 0 || v >= n) { ok = 0; break; }
            if (seen[v]) { ok = 0; break; }
            seen[v] = 1;
        }

        for (int i = 0; i < n && ok; i++) if (!seen[i]) ok = 0;
        printf("All checked: %s\n", ok ? "OK" : "FAIL");

        free(final);
        free(seen);
    }
    else MPI_Gatherv(recvbuf2, rtotal2, MPI_INT, NULL, NULL, NULL, MPI_INT, 0, MPI_COMM_WORLD);

    /* free */
    free(block_counts);
    free(block_displs);

    free(cyclic_data);

    free(local_block);

    free(recvcounts);
    free(recvdispls);
    free(recvbuf);
    free(recvbuf2);
    free(recvcounts2);
    free(recvdispls2);

    free(sendbuf);
    free(sendbuf2);
    free(sendcounts);
    free(sendcounts2);
    free(senddispls);
    free(senddispls2);

    if (my_rank == 0) {
        free(gather_counts);
        free(gather_displs);
    }

    free(times);

    MPI_Finalize();
    return 0;
}
