#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct node{
    int val;
    struct node* next;
    pthread_rwlock_t lock;
} node;

node* root = NULL;
pthread_rwlock_t root_lock = PTHREAD_RWLOCK_INITIALIZER;

int thread_count;
int total_ops = 1000;

void Insert(int val);
void Delete(int val);
node* Search(int val);
void Destroy_list();

void* worker(void* arg){
    int id = *(int*)arg;

    for (int i = 0; i < total_ops; ++i){
        int v = rand() % 10000;
        int op = rand() % 3;

        if (op == 0) Insert(v);
        else if (op == 1) Search(v);
        else Delete(v);
    }

    return NULL;
}

int main(int argc, char* argv[]){
    srand(time(NULL));

    thread_count = strtol(argv[1], NULL, 10);

    pthread_t threads[thread_count];
    int ids[thread_count];

    struct timespec start, finish;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < thread_count; ++i){
        ids[i] = i;
        pthread_create(&threads[i], NULL, worker, &ids[i]);
    }
    for (int i = 0; i < thread_count; ++i)
        pthread_join(threads[i], NULL);

    clock_gettime(CLOCK_MONOTONIC, &finish);
    double secs = (finish.tv_sec - start.tv_sec) + (finish.tv_nsec - start.tv_nsec) / 1e9;

    printf("Tiempo total: %.3f s\n", secs);

    pthread_rwlock_destroy(&root_lock);
    Destroy_list();
    return 0;
}

node* Create_node(int val){
    node* new_node = malloc(sizeof(node));
    
    new_node->val = val;
    pthread_rwlock_init(&new_node->lock, NULL);
    new_node->next = NULL;

    return new_node;
}

void Destroy_node(node* n){
    pthread_rwlock_destroy(&n->lock);
    free(n);
}

void Destroy_list(){
    node* curr = root;
    while (curr != NULL){
        node* next = curr->next;
        Destroy_node(curr);
        curr = next;
    }

    root = NULL;
}

void Insert(int val){
    pthread_rwlock_wrlock(&root_lock);

    if (root == NULL){
        root = Create_node(val);
        pthread_rwlock_unlock(&root_lock);
        return;
    }

    pthread_rwlock_rdlock(&root->lock);
    pthread_rwlock_unlock(&root_lock);

    node* curr = root;
    node* prev = NULL;

    while(curr != NULL){
        pthread_rwlock_rdlock(&curr->lock);

        if(curr->val > val) break;

        prev = curr;
        curr = curr->next;

        if (curr) pthread_rwlock_rdlock(&curr->lock);
        if (prev) pthread_rwlock_unlock(&prev->lock);
    }

    node* temp = Create_node(val);
    temp->next = curr;

    if (prev == NULL) {
        pthread_rwlock_wrlock(&root_lock);
        temp->next = root;
        root = temp;
        pthread_rwlock_unlock(&root_lock);
    }
    else {
        pthread_rwlock_wrlock(&prev->lock);
        prev->next = temp;
        pthread_rwlock_unlock(&prev->lock);
    }

    if (curr) pthread_rwlock_unlock(&curr->lock);
}

node* Search(int val){
    pthread_rwlock_rdlock(&root_lock);

    node* curr = root;

    if (curr) pthread_rwlock_rdlock(&curr->lock);
    pthread_rwlock_unlock(&root_lock);

    while (curr != NULL){
        if (curr->val == val) {
            pthread_rwlock_unlock(&curr->lock);
            return curr;
        } 

        else if (curr->val > val) break;

        node* next = curr->next;

        if (next) pthread_rwlock_rdlock(&next->lock);
        pthread_rwlock_unlock(&curr->lock);

        curr = next;
    }

    if (curr) pthread_rwlock_unlock(&curr->lock);
    return NULL;
}

void Delete(int val){
    pthread_rwlock_wrlock(&root_lock);
    
    if (root == NULL){
        pthread_rwlock_unlock(&root_lock);
        return;
    }

    pthread_rwlock_rdlock(&root->lock);
    pthread_rwlock_unlock(&root_lock);

    node* curr = root;
    node* prev = NULL;

    while (curr != NULL){
        pthread_rwlock_wrlock(&curr->lock);
        if (curr->val == val) {
            if (prev == NULL){
                pthread_rwlock_wrlock(&root_lock);
                root = curr->next;
                pthread_rwlock_unlock(&root_lock);
            }
            else {
                pthread_rwlock_wrlock(&prev->lock);
                prev->next = curr->next;
                pthread_rwlock_unlock(&prev->lock);
            }

            pthread_rwlock_unlock(&curr->lock);
            Destroy_node(curr);
            return;
        } 
        else if (curr->val > val){
            pthread_rwlock_unlock(&curr->lock);
            break;
        }
        
        if (prev) pthread_rwlock_unlock(&prev->lock);
        prev = curr;
        curr = curr->next;
    }

    if (prev) pthread_rwlock_unlock(&prev->lock);
}