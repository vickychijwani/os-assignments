/* This is an implementation of a generalized solution to the classic
 * Producers-Consumers problem, with NUM_PRODS Producers and NUM_CONS Consumers.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <error.h>
#include <errno.h>

#define BUFSIZE 20
#define NUM_MAX 50
#define NUM_PRODS 1
#define NUM_CONS 2

#define BUF_END -1

/* wait() and signal() */
#define P(s) semop(s, &pop, 1)
#define V(s) semop(s, &vop, 1)

/* Macros for convenient manipulation of shared memory (and error
   handling too) */
#define SHMGET(id, type, n) do { int shm_id = shmget(IPC_PRIVATE, ((n)*sizeof(type)), 0777 | IPC_CREAT); err(shm_id); shm_ids[++shm_count] = shm_id; id = shmat(shm_id, 0, 0); err((int) id); } while (0)
#define SHMDT(id) err(shmdt(id))
#define SHMREM() do { int i; for (i = 0; i <= shm_count; i++) err(shmctl(shm_ids[i], IPC_RMID, 0)); } while (0)

/* Macros for convenient manipulation of sempahores (and error
   handling too) */
#define SEMGET(id, val) do { id = semget(IPC_PRIVATE, 1, 0777 | IPC_CREAT); err(id); err(semctl(id, 0, SETVAL, val)); } while (0)
#define SEMGETVAL(id) semctl(id, 0, GETVAL, 0)
#define SEMREM(id) err(semctl(id, 0, IPC_RMID, 0))

/* external variables */
extern int errno;

/* global variables */
char *prog = "ProdCons";      /* for error handling */
int *buf, *head, *tail, *sum; /* shared data */
int empty, full;              /* semaphores to prevent underflow / overflow of the shared buffer */
int mutex;                    /* semaphore to enforce mutual exclusion */
int prod_count;               /* semaphore to keep track of the number of active producers */
struct sembuf pop, vop;       /* sembufs to realize wait() and signal() calls */
int shm_ids[100], shm_count = -1;

/* function prototypes */
void producer(void);
void consumer(void);
int buf_dequeue(void);
void buf_enqueue(int);
void shm_cleanup(void);
void err(int);


int main(void) {
    int i, status;

    SEMGET(empty, BUFSIZE);
    SEMGET(full, 0);
    SEMGET(mutex, 1);
    SEMGET(prod_count, 0);

    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1; vop.sem_op = 1;

    SHMGET(buf, int, BUFSIZE);
    SHMGET(head, int, 1);
    SHMGET(tail, int, 1);
    SHMGET(sum, int, 1);

    *head = *tail = *sum = 0;

    for (i = 0; i < NUM_PRODS; i++)
        producer();

    for (i = 0; i < NUM_CONS; i++)
        consumer();

    while (wait(&status) > 0)
        ;

    printf("SUM = %d\n", *sum);

    shm_cleanup();

    SEMREM(full);
    SEMREM(empty);
    SEMREM(mutex);
    SEMREM(prod_count);

    return 0;
}

void producer(void) {
    if (fork() == 0) {
        int i;

        V(prod_count);

        for (i = 1; i <= NUM_MAX; i++)
            buf_enqueue(i);

        P(prod_count);

        if (!SEMGETVAL(prod_count))
            buf_enqueue(BUF_END);

        exit(0);
    }
}

void consumer(void) {
    if (fork() == 0) {
        int num;

        while (1) {
            num = buf_dequeue();

            P(mutex);
            *sum += num;
            V(mutex);
        }

        exit(0);
    }
}

int buf_dequeue(void) {
    int temp;

    P(full);
    P(mutex);

    temp = buf[*head];
    if (temp == BUF_END) {
        V(mutex);
        V(full);
        exit(0);
    }
    *head = ((*head) + 1) % BUFSIZE;

    V(mutex);
    V(empty);

    return temp;
}

void buf_enqueue(int num) {
    P(empty);
    P(mutex);

    buf[*tail] = num;
    *tail = ((*tail) + 1) % BUFSIZE;

    V(mutex);
    V(full);
}

void shm_cleanup(void) {
    SHMDT(buf);
    SHMDT(head);
    SHMDT(tail);
    SHMDT(sum);
    SHMREM();
}

void err(int ret) {
    if (ret == -1) {
        perror(prog);
        exit(-1);
    }
}
