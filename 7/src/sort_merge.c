#include <stdio.h>
#include <pthread.h>
#include "mem_mgmt.h"


#define NTHREADS 2
#define malloc mem_malloc
#define free mem_free


/* structures for BST */

typedef struct _node {
    int data;
    struct _node * left;
    struct _node * right;
} node;


void * tmain(void *);

void do_work(void);

void init_mutexes(void);
void init_workers(int param[]);

void wind_up(void);

void process_node(node * n, int sorted[], int * i);

/* BST-related functions */
void tree_insert(node ** t, int k, int * count);
void tree_in_order_walk(node * t, int sorted[], int * i);

pthread_mutex_t mem_mutex;
pthread_t tid[NTHREADS];
int * sorted[NTHREADS];
int count[NTHREADS];

int main(void) {
    int i, param[NTHREADS];

    for (i = 0; i < NTHREADS; i++) {
        param[i] = i;
    }

    init_mem();
    init_mutexes();
    init_workers(param);

    do_work();

    wind_up();

    return 0;
}

void * tmain(void * tno) {
    int i = * (int *) tno, num, n = 0;
    char * file_name_format = "input%d.txt", file_name[11];
    FILE * fp;
    node * tree = NULL;

    sprintf(file_name, file_name_format, i + 1);
    fp = fopen(file_name, "r");

    count[i] = 0;
    while (fscanf(fp, "%d", &num) != EOF) {
        tree_insert(&tree, num, &count[i]);
    }

    fclose(fp);

    sorted[i] = malloc(count[i] * sizeof(int));

    tree_in_order_walk(tree, sorted[i], &n);

    pthread_exit(NULL);
}

void do_work(void) {
    int i, * p1, * p2, prev, flag = 0;
    FILE * fp;

    /* wait for workers to exit */
    for (i = 0; i < NTHREADS; i++) {
        if (pthread_join(tid[i], NULL))
            fprintf(stderr, "Master: Unable to wait for worker %d\n", i);
        else
            printf("Master: Worker %d has exited\n", i);
    }

    /* merge sorted arrays */
    printf("array 1: ");
    for (i = 0; i < count[0]; i++) {
        printf("%d ", sorted[0][i]);
    }
    printf("\n");

    printf("array 2: ");
    for (i = 0; i < count[1]; i++) {
        printf("%d ", sorted[1][i]);
    }
    printf("\n");

    printf("Merging...\n");

    fp = fopen("output.txt", "w");

    p1 = sorted[0];
    p2 = sorted[1];
    while (count[0] > 0 && count[1] > 0) {
        if (*p1 < *p2) {
            if (*p1 == prev && flag) {
                p1++;
                count[0]--;
                continue;
            }

            fprintf(fp, "%d\n", *p1);
            flag = 1;
            prev = *p1;
            p1++;
            count[0]--;
        }
        else if (*p1 > *p2) {
            if (*p2 == prev && flag) {
                p2++;
                count[1]--;
                continue;
            }

            fprintf(fp, "%d\n", *p2);
            flag = 1;
            prev = *p2;
            p2++;
            count[1]--;
        }
        else {
            p1++;
            count[0]--;
            continue;
        }
    }

    flag = 0;
    while (count[0] > 0) {
        if (*p1 == prev && flag) {
            p1++;
            count[0]--;
        }

        fprintf(fp, "%d\n", *p1);
        flag = 1;
        prev = *p1;
        p1++;
        count[0]--;
    }
    while (count[1] > 0) {
        if (*p2 == prev && flag) {
            p2++;
            count[1]--;
        }

        fprintf(fp, "%d\n", *p2);
        flag = 1;
        prev = *p2;
        p2++;
        count[1]--;
    }

    fclose(fp);

    free(sorted[0]);
    free(sorted[1]);
}

void init_mutexes(void) {
    pthread_mutex_init(&mem_mutex, NULL);

    /* ensure mutexes are unlocked */
    pthread_mutex_trylock(&mem_mutex);
    pthread_mutex_unlock(&mem_mutex);
}

void init_workers(int param[]) {
    int i;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    for (i = 0; i < NTHREADS; i++) {
        param[i] = i;
        if (pthread_create(&tid[i], &attr, tmain, (void *) &param[i])) {
            fprintf(stderr, "Master: Unable to create worker %d. Exiting...\n", param[i]);
            pthread_attr_destroy(&attr);
            exit(1);
        }

        printf("Master: Worker %d created\n", param[i]);
    }

    pthread_attr_destroy(&attr);
}


void wind_up(void) {
    printf("Winding up...\n");
    pthread_mutex_destroy(&mem_mutex);
}


void process_node(node * n, int sorted[], int * i) {
    if (! n)
        return;

    sorted[*i] = n->data;
    (*i) = (*i) + 1;
}


/* BST-related functions */
void tree_insert(node ** t, int k, int * count) {
    if((*t) == NULL) {
        pthread_mutex_lock(&mem_mutex);
        (*t) = malloc(sizeof(node));
        pthread_mutex_unlock(&mem_mutex);
        (*t)->left = NULL;
        (*t)->right = NULL;
        (*t)->data = k;
        (*count) = (*count) + 1;
    }
    else if((*t)->data >= k) {
        tree_insert(&((*t)->left), k, count);
    }
    else {
        tree_insert(&((*t)->right), k, count);
    }
}

void tree_in_order_walk(node * t, int sorted[], int * i) {
    if (t == NULL)
        return;

    tree_in_order_walk(t->left, sorted, i);
    process_node(t, sorted, i);
    tree_in_order_walk(t->right, sorted, i);

    pthread_mutex_lock(&mem_mutex);
    free(t);
    pthread_mutex_unlock(&mem_mutex);
}
