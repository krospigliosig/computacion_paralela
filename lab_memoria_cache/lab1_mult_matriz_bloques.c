#include <stdio.h>

#define N 8
#define BLOCK_SIZE 4

void print_matrix(int M[N][N]) {
    for (int i = 0; i < N; ++i){
        for (int j = 0; j < N; ++j)
            printf("%4d ", M[i][j]);
        printf("\n");
    }
    printf("\n");
}

int main() {
    double A[N][N], B[N][N], C[N][N];

    /* Inicializar A, B*/

    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j){
            A[i][j] = i + j;
            B[i][j] = i + j;
        }

    print_matrix(A);
    print_matrix(B);

    /* Asignar C = 0 */
    memset(C, 0, sizeof(C));

    /* C = A x B */
    for (int ii = 0; ii < N; ii += BLOCK_SIZE)
        for (int jj = 0; jj < N; jj += BLOCK_SIZE)
            for (int kk = 0; kk < N; kk += BLOCK_SIZE)

                for (int i = ii; i < ii + BLOCK_SIZE; ++i)
                    for (int j = jj; j < jj + BLOCK_SIZE; ++j)
                        for (int k = kk; k < kk + BLOCK_SIZE; ++k)
                            C[i][j] += A[i][k] * B[k][j];

    /* Imprimir C */
    print_matrix(C);
} 

