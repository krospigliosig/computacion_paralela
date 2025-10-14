#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

typedef struct node{
    int val;
    struct node* next;
} node;

typedef struct {
    double insert_pct;
    double delete_pct;
    double search_pct;
} op_ratio_t;

op_ratio_t ratios = {0.05, 0.05, 0.9};

node* root = NULL;
pthread_rwlock_t root_lock = PTHREAD_RWLOCK_INITIALIZER;

int thread_count = 1;
int total_ops = 10000;

void Insert(int val);
void Delete(int val);
bool Search(int val);
void Destroy_list();

void* worker(void* arg){
    int id = *(int*)arg;
    unsigned int seed = (unsigned int)time(NULL) ^ (unsigned int)(id * 7919);

    for (int i = 0; i < total_ops; ++i){
        int v = rand_r(&seed) % 10000;
        double r = (double)rand_r(&seed) / RAND_MAX;

        if (r < ratios.insert_pct)
            Insert(v);
        else if (r < ratios.insert_pct + ratios.search_pct)
            Search(v);
        else
            Delete(v);
    }

    return NULL;
}

int main(int argc, char* argv[]){
    if (argc < 2){
        fprintf(stderr, "Uso: %s <thread_count>\n", argv[0]);
        return 1;
    }

    thread_count = (int)strtol(argv[1], NULL, 10);
    if (thread_count <= 0) thread_count = 1;

    pthread_t* threads = malloc(sizeof(pthread_t) * thread_count);
    int* ids = malloc(sizeof(int) * thread_count);

    struct timespec start, finish;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < thread_count; ++i){
        ids[i] = i;
        if (pthread_create(&threads[i], NULL, worker, &ids[i]) != 0){
            perror("pthread_create");
            return 1;
        }
    }
    for (int i = 0; i < thread_count; ++i){
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &finish);
    double secs = (finish.tv_sec - start.tv_sec) + (finish.tv_nsec - start.tv_nsec) / 1e9;

    printf("Tiempo total: %e s\n", secs);

    Destroy_list();
    free(threads);
    free(ids);

    pthread_rwlock_destroy(&root_lock);
    return 0;
}

node* Create_node(int val){
    node* new_node = malloc(sizeof(node));
    if (!new_node) {
        perror("malloc");
        exit(1);
    }

    new_node->val = val;
    new_node->next = NULL;
    return new_node;
}

void Destroy_node(node* n){
    free(n);
}

void Destroy_list(){
    pthread_rwlock_wrlock(&root_lock);
    node* curr = root;

    while (curr != NULL){
        node* next = curr->next;
        Destroy_node(curr);
        curr = next;
    }

    root = NULL;
    pthread_rwlock_unlock(&root_lock);
}

void Insert(int val){
    pthread_rwlock_wrlock(&root_lock);

    if (root == NULL || root->val > val){
        node* temp = Create_node(val);

        temp->next = root;
        root = temp;

        pthread_rwlock_unlock(&root_lock);
        return;
    }

    node* prev = root;
    node* curr = root->next;

    while (curr != NULL && curr->val <= val){
        prev = curr;
        curr = curr->next;
    }

    node* temp = Create_node(val);
    temp->next = curr;
    prev->next = temp;

    pthread_rwlock_unlock(&root_lock);
}

bool Search(int val){
    pthread_rwlock_rdlock(&root_lock);

    node* curr = root;
    while (curr != NULL){
        if (curr->val == val){
            pthread_rwlock_unlock(&root_lock);
            return true;
        }
        else if (curr->val > val) break;

        curr = curr->next;
    }

    pthread_rwlock_unlock(&root_lock);
    return false;
}

void Delete(int val){
    pthread_rwlock_wrlock(&root_lock);

    if (root == NULL){
        pthread_rwlock_unlock(&root_lock);
        return;
    }

    if (root->val == val){
        node* to_free = root;
        root = root->next;

        Destroy_node(to_free);
        pthread_rwlock_unlock(&root_lock);

        return;
    }

    node* prev = root;
    node* curr = root->next;

    while (curr != NULL && curr->val < val){
        prev = curr;
        curr = curr->next;
    }

    if (curr != NULL && curr->val == val){
        prev->next = curr->next;
        Destroy_node(curr);
    }

    pthread_rwlock_unlock(&root_lock);
}
