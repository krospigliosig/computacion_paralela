#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM_SIZES 8
const int N[NUM_SIZES] = { 16, 32, 64, 128, 256, 512, 1024, 2048 };
#define BLOCK_SIZE 4

void mat_init(double** A, double** B, double** C, int n) {
    for (int i = 0; i < n; i++) {
        A[i] = (double*)malloc(n * sizeof(double));
        B[i] = (double*)malloc(n * sizeof(double));
        C[i] = (double*)malloc(n * sizeof(double));
        for (int j = 0; j < n; j++)
            C[i][j] = 0.0;
    }
}

void mat_fill(double** A, double** B, int n) {
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j) {
            A[i][j] = rand() % 100;
            B[i][j] = rand() % 100;
        }
}

void mat_free(double** A, double** B, double** C, int n) {
    for (int i = 0; i < n; i++) {
        free(A[i]);
        free(B[i]);
        free(C[i]);
    }
    free(A);
    free(B);
    free(C);
}

void mat_mult_clasic(double** A, double** B, double** C, int n) {
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
            for (int k = 0; k < n; ++k)
                C[i][j] += A[i][k] * B[k][j];
}

void mat_mult_blocks(double** A, double** B, double** C, int n) {
    for (int ii = 0; ii < n; ii += BLOCK_SIZE)
        for (int jj = 0; jj < n; jj += BLOCK_SIZE)
            for (int kk = 0; kk < n; kk += BLOCK_SIZE)

                for (int i = ii; i < ii + BLOCK_SIZE && i < n; ++i)
                    for (int j = jj; j < jj + BLOCK_SIZE && j < n; ++j)
                        for (int k = kk; k < kk + BLOCK_SIZE && k < n; ++k)
                            C[i][j] += A[i][k] * B[k][j];
}

int main() {
    srand(time(NULL));

    FILE *f = fopen("data_clasic.dat", "w");
    if (!f) {
        printf("Error opening file!\n");
        return 1;
    }

    for(int i = 0; i < NUM_SIZES; i++) {
        int n = N[i];
        double **A = (double**)malloc(n * sizeof(double*));
        double **B = (double**)malloc(n * sizeof(double*));
        double **C = (double**)malloc(n * sizeof(double*));

        mat_init(A, B, C, n);
        mat_fill(A, B, n);

        clock_t start = clock();
        mat_mult_clasic(A, B, C, n);
        clock_t end = clock();

        double time = (double)(end - start) / CLOCKS_PER_SEC;
        fprintf(f, "%d %f\n", n, time);

        mat_free(A, B, C, n);
    }

    fclose(f);

    f = fopen("data_blocks.dat", "w");
    if (!f) {
        printf("Error opening file!\n");
        return 1;
    }

    for(int i = 0; i < NUM_SIZES; i++) {
        int n = N[i];
        double **A = (double**)malloc(n * sizeof(double*));
        double **B = (double**)malloc(n * sizeof(double*));
        double **C = (double**)malloc(n * sizeof(double*));

        mat_init(A, B, C, n);
        mat_fill(A, B, n);

        clock_t start = clock();
        mat_mult_blocks(A, B, C, n);
        clock_t end = clock();

        double time = (double)(end - start) / CLOCKS_PER_SEC;
        fprintf(f, "%d %f\n", n, time);

        mat_free(A, B, C, n);
    }

    fclose(f);
    return 0;
}
