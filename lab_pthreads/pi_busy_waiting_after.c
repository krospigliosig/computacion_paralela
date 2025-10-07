#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int flag;
long thread_count;
long long n;
double sum;

void *Thread_sum(void* rank){
    long my_rank = (long) rank;
    double factor, my_sum = 0.0;
    long long i;
    long long my_n = n / thread_count;
    long long my_first_i = my_n * my_rank;
    long long my_last_i = my_first_i + my_n;

    if (my_first_i % 2 == 0)
        factor = 1.0;
    else factor = -1.0;

    for (i = my_first_i; i < my_last_i; ++i, factor = -factor)
        my_sum += factor / (2 * i + 1);

    while (flag != my_rank);
    sum += my_sum;
    flag = (flag + 1) % thread_count;

    return NULL;
}

int main(int argc, char* argv[]){
    long thread;
    pthread_t* thread_handles;

    thread_count = strtol(argv[1], NULL, 10);
    n = strtoll(argv[2], NULL, 10);

    thread_handles = malloc(thread_count * sizeof(pthread_t));

    sum = 0.0;
    flag = 0;

    for(thread = 0; thread < thread_count; ++thread)
        pthread_create(&thread_handles[thread], NULL, Thread_sum, (void*) thread);

    printf("thread principal\n");

    for(thread = 0; thread < thread_count; ++thread)
        pthread_join(thread_handles[thread], NULL);

    sum *= 4.0;
    printf("Valor de pi para n=%lld: %lf\n", n, sum);

    free(thread_handles);
    return 0;
}