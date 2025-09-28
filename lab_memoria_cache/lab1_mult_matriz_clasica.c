#include <stdio.h>

#define I 100
#define J 100
#define K 100

int main() {
    double A[I][J], B[J][K], C[I][K];

    /* Inicializar A, B*/
    for (int i = 0; i < I; ++i)
        for (int j = 0; j < J; ++j)
            A[i][j] = i + j;

    for (int j = 0; j < J; ++j)
        for (int k = 0; k < K; ++k)
            B[j][k] = j + k;

    /* Asignar C = 0 */
    memset(C, 0, sizeof(C));

    /* C = A x B */

    for (int i = 0; i < I; ++i)
        for (int k = 0; k < K; ++k)
            for (int j = 0; j < J; ++j)
                C[i][k] += A[i][j] * B[j][k];
}