#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>

/* constants */
#define M 1000 /* size of A */
#define N 4    /* no. of workers */
#define P 0.5f /* probability that A[i][j] = 1 */

/* synchronization possibilities */
#define ALL 1
#define WORKERS 2

/* function prototypes */
void do_work(pthread_t *, int *);

void *tmain(void *);
void synchronize(unsigned char);

void init_A(void);
void print_A(void);
void partial_square_A(int, int);
void partial_copy_B_to_A(int, int);

void init_mutexes(void);
void init_workers(pthread_t *, int *);
void wind_up(void);
void cleanup_and_exit(int);

/* global variables (data shared among all worker threads) */
static unsigned char A[M][M], B[M][M];
static pthread_mutex_t w_mutex, mw_mutex, one_count_mutex;
static pthread_cond_t w_sync, mw_sync;
static unsigned int one_count = 0;
static int w_done = 0;
static int closure_done = 0;

int main(void) {
    pthread_t tid[N];
    int param[N];

    signal(SIGINT, cleanup_and_exit);
    signal(SIGTERM, cleanup_and_exit);

    init_mutexes();
    init_A();
    print_A();
    init_workers(tid, param);

    do_work(tid, param);

    wind_up();

    return 0;
}

void *tmain(void *tno) {
    int i, j, n = * (int *) tno, count = 0, e = 0;
    int start = (int) ((n - 1) * M) / N, end = (int) (n * M) / N - 1; /* n belongs to [1, N] */

    /* count ones */
    for (i = start; i <= end; i++)
        for (j = 0; j < M; j++)
            if (A[i][j])
                count++;

    pthread_mutex_lock(&one_count_mutex);
    one_count += count;
    pthread_mutex_unlock(&one_count_mutex);

    /* sync workers, last worker wakes up master */
    synchronize(ALL);

    /* compute transitive closure */
    while ((1 << e) < M) { /* 1 << e == 2^e */
        partial_square_A(start, end);
        synchronize(WORKERS);

        partial_copy_B_to_A(start, end);
        synchronize(WORKERS);

        e++;
    }

    if (n == N)
        closure_done = 1;

    synchronize(ALL);

    /* printf("Worker %d exiting...\n", n); */
    pthread_exit(NULL);
}

void synchronize(unsigned char flag) {
    if (flag == WORKERS || flag == ALL) {
        pthread_mutex_lock(&w_mutex);

        w_done++;

        if (w_done < N) {
            pthread_cond_wait(&w_sync, &w_mutex);
        }

        else {
            w_done = 0;
            pthread_cond_broadcast(&w_sync);
            if (flag == ALL) {
                pthread_mutex_lock(&mw_mutex);
                /* printf("Waking up master...\n"); */
                pthread_cond_broadcast(&mw_sync);
                pthread_mutex_unlock(&mw_mutex);
            }
        }

        pthread_mutex_unlock(&w_mutex);
    }
}

void do_work(pthread_t *tid, int *param) {
    int i;

    /* wait for workers to finish part 3 (counting the no. of ones in A) */
    /* printf("Master waiting (cond 1)\n"); */
    pthread_cond_wait(&mw_sync, &mw_mutex);
    /* print results of part 3 */
    printf("\nMaster: No. of ones in array = %u\n\n", one_count);

    /* wait for workers to finish part 4 (computing transitive closure of A) */
    /* printf("Master waiting (cond 2)\n"); */
    if (! closure_done)
        pthread_cond_wait(&mw_sync, &mw_mutex);
    /* print results of part 4 */
    printf("Master: Transitive closure:\n");
    print_A();
    pthread_mutex_unlock(&mw_mutex);

    /* wait for workers to exit */
    for (i = 0; i < N; i++) {
        if (pthread_join(tid[i], NULL))
            fprintf(stderr, "Master: Unable to wait for worker %d\n", param[i]);
        else
            printf("Master: Worker %d has exited\n", param[i]);
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

void partial_square_A(int start, int end) {
    int i, j, k;

    for (i = start; i <= end; i++) {
        for (j = 0; j < M; j++) {
            B[i][j] = 0;
            for (k = 0; k < M; k++) {
                if (A[i][k] && A[k][j]) {
                    B[i][j] = 1;
                    break;
                }
            }
        }
    }
}

void partial_copy_B_to_A(int start, int end) {
    int i, j;

    for (i = start; i <= end; i++) {
        for (j = 0; j < M; j++) {
            A[i][j] = B[i][j];
        }
    }
}

void wind_up(void) {
    /* Destroy mutexes and condition variables */
    printf("Winding up...\n");

    pthread_cond_destroy(&w_sync);
    pthread_cond_destroy(&mw_sync);

    pthread_mutex_destroy(&mw_mutex);
    pthread_mutex_destroy(&w_mutex);
    pthread_mutex_destroy(&one_count_mutex);
}

void cleanup_and_exit(int signal) {
    if (signal == SIGINT)
        printf("Received SIGINT...\n");
    if (signal == SIGTERM)
        printf("Received SIGTERM...\n");
    wind_up();
}
