#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

/* constants */
#define M 10 /* size of A */
#define N 4    /* no. of workers */
#define P 0.5f /* probability that A[i][j] = 1 */

/* synchronization possibilities */
#define WORKERS 1
#define MASTER_WORKER 2

/* comment the following to hide debugging info */
/* #define DEBUG */

/* function prototypes */
void do_work(pthread_t *, int *);

void *tmain(void *);
void synchronize(unsigned char);

void init_A(void);
void print_A(void);
void print_B(void);
void partial_square_A(int, int);
void partial_copy_B_to_A(int, int);

#ifdef DEBUG
void read_mat_from_file(void);
#endif
void write_A_to_file(void);

void init_mutexes(void);
void init_workers(pthread_t *, int *);
void wind_up(void);

/* global variables (data shared among all worker threads) */
static unsigned char A[M][M], B[M][M];
static pthread_mutex_t w_mutex, mw_mutex, one_count_mutex;
static pthread_cond_t w_sync, mw_sync;
static unsigned int one_count = 0;
static char w_done = 0;

int main(void) {
    #ifdef DEBUG
    /* setbuf(stdout, NULL); */
    #endif
    pthread_t tid[N];
    int param[N];

    init_mutexes();
    init_A();
    /* write_A_to_file(); */
    /* read_mat_from_file(); */
    print_A();
    init_workers(tid, param);

    do_work(tid, param);

    wind_up();

    return 0;
}

void *tmain(void *tno) {
    int i, j, n = * (int *) tno, count = 0, e = 0;
    //float l = M * 1.0f / N;
    int start = (int) ((n - 1) * M) / N, end = (int) (n * M) / N - 1; /* n belongs to [1, N] */

    printf("Worker %d: Running...\n", n);

    /* count ones */
    for (i = start; i <= end; i++)
        for (j = 0; j < M; j++)
            if (A[i][j])
                count++;

    pthread_mutex_lock(&one_count_mutex);
    one_count += count;
    pthread_mutex_unlock(&one_count_mutex);

    /* sync workers, last worker wakes up master */
    /* synchronize(WORKERS | MASTER_WORKER); */
    synchronize(1);

    /* compute transitive closure */
    while ((1 << e) < M) { /* 1 << e == 2^e */
        partial_square_A(start, end);
        synchronize(2);

        partial_copy_B_to_A(start, end);
        synchronize(3);

        e++;
    }

    #ifdef DEBUG
    printf("Worker %d: transitive closure done\n", n);
    #endif
    printf("");
    synchronize(1);

    #ifdef DEBUG
    #endif
    printf("Worker %d: Exiting...\n", n);
    printf("");
    pthread_exit(NULL);
}

void synchronize(unsigned char flag) {
    pthread_mutex_lock(&w_mutex);

    w_done++;

    if (w_done < N) {
        pthread_cond_wait(&w_sync, &w_mutex);
    }

    else {
        /* #ifdef DEBUG */
        /* if (flag == 2) */
        /*     printf("square done\n"); */
        /* else if (flag == 3) */
        /*     printf("copy done\n"); */
        /* printf("B = \n"); */
        /* print_B(); */
        /* #endif */
        printf("");
        w_done = 0;
        pthread_cond_broadcast(&w_sync);
        if (flag) {
            pthread_mutex_lock(&mw_mutex);
            pthread_cond_broadcast(&mw_sync);
            pthread_mutex_unlock(&mw_mutex);
        }
    }

    pthread_mutex_unlock(&w_mutex);
}

/* void synchronize(unsigned char flag) { */
/*     pthread_mutex_lock(&w_mutex); */
/*     w_done++; */
/*     pthread_mutex_unlock(&w_mutex); */

/*     printf("flag = %hhu\n", flag); */

/*     if (flag & WORKERS) { */
/*         pthread_mutex_lock(&w_mutex); */

/*         if (w_done < N) { */
/*             pthread_cond_wait(&w_sync, &w_mutex); */
/*         } */
/*         else { */
/*             /\* printf("syncing workers...\n"); *\/ */
/*             pthread_cond_broadcast(&w_sync); */
/*         } */
/*         pthread_mutex_unlock(&w_mutex); */
/*     } */

/*     if (flag & MASTER_WORKER) { */
/*         if (w_done == N) { */
/*             /\* printf("syncing master and workers...\n"); *\/ */
/*             pthread_mutex_lock(&mw_mutex); */
/*             pthread_cond_broadcast(&mw_sync); */
/*             pthread_mutex_unlock(&mw_mutex); */
/*         } */
/*     } */

/*     pthread_mutex_lock(&w_mutex); */
/*     if (w_done == N) */
/*         w_done = 0; */
/*     pthread_mutex_unlock(&w_mutex); */
/* } */

void do_work(pthread_t *tid, int *param) {
    int i;

    /* wait for workers to finish part 3 (counting the no. of ones in A) */
    pthread_cond_wait(&mw_sync, &mw_mutex);
    /* print results of part 3 */
    printf("\nMaster: No. of ones in array = %u\n\n", one_count);

    /* wait for workers to finish part 4 (computing transitive closure of A) */
    pthread_cond_wait(&mw_sync, &mw_mutex);
    /* print results of part 4 */
    printf("Transitive closure :\n");
    print_A();
    pthread_mutex_unlock(&mw_mutex);

    /* wait for workers to exit */
    for (i = 0; i < N; i++) {
        if (pthread_join(tid[i], NULL))
            fprintf(stderr, "Master: Unable to wait for worker %d\n", param[i]);
        else {
            printf("Master: Worker %d has exited\n", param[i]);
        }
    }
}

void init_mutexes(void) {
    /* initialize mutexes */
    pthread_mutex_init(&w_mutex, NULL);
    pthread_mutex_init(&mw_mutex, NULL);
    pthread_mutex_init(&one_count_mutex, NULL);

    /* ensure mutexes are unlocked, irrespective of implementation */
    pthread_mutex_trylock(&w_mutex);
    pthread_mutex_unlock(&w_mutex);

    pthread_mutex_trylock(&mw_mutex);
    pthread_mutex_unlock(&mw_mutex);

    pthread_mutex_trylock(&one_count_mutex);
    pthread_mutex_unlock(&one_count_mutex);

    /* initialize condition variables */
    pthread_cond_init(&w_sync, NULL);
    pthread_cond_init(&mw_sync, NULL);
}

void init_workers(pthread_t *tid, int *param) {
    pthread_attr_t attr;
    int i;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    pthread_mutex_lock(&mw_mutex);
    for (i = 0; i < N; i++) {
        param[i] = i + 1;

        if (pthread_create(&tid[i], &attr, tmain, (void *) &param[i])) {
            fprintf(stderr, "Master: Unable to create worker %d. Exiting...\n", param[i]);
            pthread_attr_destroy(&attr);
            exit(1);
        }

        printf("Master: Worker %d created\n", param[i]);
    }

    pthread_attr_destroy(&attr);
}

void init_A(void) {
    int i, j;
    float r;

    srand((unsigned int) time(NULL));
    for (i = 0; i < M; i++) {
        for (j = 0; j < M; j++) {
            r = (rand() * 1.0) / RAND_MAX;
            if (r < P) A[i][j] = 1;
            else       A[i][j] = 0;
        }
    }
}

void print_A(void) {
    int i, j;

    for (i = 0; i < M; i++) {
        for (j = 0; j < M; j++) {
            printf("%d ", A[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

void print_B(void) {
    int i, j;

    for (i = 0; i < M; i++) {
        for (j = 0; j < M; j++) {
            printf("%d ", B[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

void partial_square_A(int start, int end) {
    int i, j, k;
    #ifdef DEBUG
    int count = 0;
    #endif

    for (i = start; i <= end; i++) {
        for (j = 0; j < M; j++) {
            B[i][j] = 0;
            for (k = 0; k < M; k++) {
                if (A[i][k] && A[k][j]) {
                    B[i][j] = 1;
                    #ifdef DEBUG
                    printf("B[%d][%d] = %d\n", i, j, B[i][j]);
                    count++;
                    #endif
                    printf("");
                    break;
                }
            }
        }
    }

    #ifdef DEBUG
    printf("count = %d\n", count);
    #endif
    printf("");
}

void partial_copy_B_to_A(int start, int end) {
    int i, j;

    for (i = start; i <= end; i++) {
        for (j = 0; j < M; j++) {
            #ifdef DEBUG
            printf("A[%d][%d] = %d\n", i, j, B[i][j]);
            #endif
            printf("");
            A[i][j] = B[i][j];
        }
    }
    printf("");
}

#ifdef DEBUG
void read_mat_from_file(void) {
    FILE *fp = fopen("input", "r");
    unsigned char n;
    int i, j;

    for (i = 0; i < M; i++) {
        for (j = 0; j < M; j++) {
            fscanf(fp, "%hhu", &n);
            A[i][j] = n;
        }
    }

    fclose(fp);
}
#endif

void write_A_to_file(void) {
    int i, j;
    FILE *fp = fopen("input", "w");

    for (i = 0; i < M; i++) {
        for (j = 0; j < M; j++) {
            fprintf(fp, "%d ", A[i][j]);
        }
        fprintf(fp, "\n");
    }

    fclose(fp);
}

void wind_up(void) {
    /* Destroy mutexes and condition variables */
    printf("Winding up...\n");

    pthread_mutex_destroy(&mw_mutex);
    pthread_mutex_destroy(&w_mutex);
    pthread_mutex_destroy(&one_count_mutex);

    pthread_cond_destroy(&w_sync);
    pthread_cond_destroy(&mw_sync);
}
