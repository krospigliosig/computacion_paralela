#include <stdio.h>

#define MAX 8

int main(){
    double A[MAX][MAX], x[MAX], y[MAX];

    /* Initialize A and x, assign y = 0 */

    for (int i = 0; i < MAX; ++i) {
        x[i] = i;
        for (int j = 0; j < MAX; ++j)
            A[i][j] = i + j;
    }

    memset(y, 0, sizeof(y));

    /* First pair of loops */
    for (int i = 0; i < MAX; ++i)
        for (int j = 0; j < MAX; ++j)
            y[i] += A[i][j] * x[j];

    /* Assign y = 0 */
    memset(y, 0, sizeof(y));

    /* Second pair of loops */
    for (int j = 0; j < MAX; ++j)
        for (int i = 0; i < MAX; ++i)
            y[i] += A[i][j] * x[j];
}