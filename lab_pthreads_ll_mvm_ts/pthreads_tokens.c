#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_MAX 1000

int thread_count;
int curr_rank = 0;
pthread_mutex_t mutex;
pthread_cond_t cond;

void* worker(void* my_rank){
    long rank = (long) my_rank;
    char my_line[BUFFER_MAX];
    char* my_string, *saveptr;
    int count;
    char* input;

    while(1){
        pthread_mutex_lock(&mutex);
        while (curr_rank != rank)
            pthread_cond_wait(&cond, &mutex);

        input = fgets(my_line, BUFFER_MAX, stdin);
        if (input == NULL) {
            curr_rank = (curr_rank + 1) % thread_count;
            pthread_cond_broadcast(&cond);
            pthread_mutex_unlock(&mutex);
            break;
        }

        printf("Thread %ld > línea = %s", rank, my_line);
        curr_rank = (curr_rank + 1) % thread_count;
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&mutex);

        count = 0;
        my_string = strtok_r(my_line, " \t\n", &saveptr);
        while (my_string != NULL) {
            count++;
            printf("Thread %ld > string %d = %s\n", rank, count, my_string);
            my_string = strtok_r(NULL, " \t\n", &saveptr);
        }
    }

    return NULL;
}

int main(int argc, char* argv[]){
    long thread;
    pthread_t* thread_handles; 

    if (argc != 2){
        fprintf(stderr, "Uso: %s <thread_count>\n", argv[0]);
        return 1;
    }

    thread_count = atoi(argv[1]);
    thread_handles = malloc(thread_count * sizeof(pthread_t));
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

    printf("Escribe una línea de texto\n");

    for (thread = 0; thread < thread_count; ++thread)
        pthread_create(&thread_handles[thread], NULL, worker, (void*) thread);

    for (thread = 0; thread < thread_count; ++thread)
        pthread_join(thread_handles[thread], NULL);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    free(thread_handles);

    return 0;
}

