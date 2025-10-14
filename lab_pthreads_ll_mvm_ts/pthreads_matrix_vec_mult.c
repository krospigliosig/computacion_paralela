#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

double** A;
double* x;
double* y;

int thread_count;
long n_rows;
long n_cols;

void Gen_A();

void Gen_x();

void Init_y();

typedef struct {
    int my_rank;
    int start_row;
    int end_row;
} ThreadData;

void* worker(void* arg) {
    ThreadData* data = (ThreadData*)arg;

    struct timespec start, finish;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = data->start_row; i < data->end_row; i++) {
        y[i] = 0.0;
        for (int j = 0; j < n_cols; j++) {
            y[i] += A[i][j] * x[j];
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &finish);
    double secs = (finish.tv_sec - start.tv_sec) + (finish.tv_nsec - start.tv_nsec) / 1e9;

    printf("Thread %d: %e s\n", data->my_rank, secs);

    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 4){
        fprintf(stderr, "Uso: %s <thread_count> <n_rows> <n_cols>\n", argv[0]);
        return 1;
    }

    thread_count = (int)strtol(argv[1], NULL, 10);
    if (thread_count <= 0) thread_count = 1;

    n_rows = strtol(argv[2], NULL, 10);
    n_cols = strtol(argv[3], NULL, 10);

    Gen_A();
    Gen_x();
    Init_y();

    int rows_per_thread = n_rows / thread_count;
    int remaining = n_rows % thread_count;
    int current_row = 0;

    pthread_t* threads = malloc(sizeof(pthread_t) * thread_count);    
    ThreadData* thread_data = malloc(sizeof(ThreadData) * thread_count);

    for (int t = 0; t < thread_count; t++) {
        int rows = rows_per_thread + (t < remaining ? 1 : 0);
        thread_data[t].start_row = current_row;
        thread_data[t].end_row = current_row + rows;
        thread_data[t].my_rank = t;
        current_row += rows;

        pthread_create(&threads[t], NULL, worker, &thread_data[t]);
    }

    for (int t = 0; t < thread_count; t++)
        pthread_join(threads[t], NULL);

    for (int i = 0; i < n_rows; i++)
        free(A[i]);
    free(A);
    free(x);
    free(y);
    free(threads);
    free(thread_data);

    return 0;
}

void Gen_A(){
    A = (double**)malloc(n_rows * sizeof(double*));
    
    for(int i = 0; i < n_rows; ++i) {
        A[i] = (double*)malloc(n_cols * sizeof(double));
        
        for(int j = 0; j < n_cols; ++j)
            A[i][j] = (double)rand() / RAND_MAX;
    }
}

void Gen_x(){
    x = (double*)malloc(n_cols * sizeof(double));
    
    for(int i = 0; i < n_cols; ++i)
        x[i] = (double)rand() / RAND_MAX;
}

void Init_y(){
    y = (double*)malloc(n_rows * sizeof(double));
}