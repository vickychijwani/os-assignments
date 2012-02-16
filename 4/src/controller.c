#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>

#include "mympl.h"

/* constants */
#define CLEANUP_INTERVAL (1*60) /* time in seconds */
#define CLEANUP_AGE (5*60)

/* WAIT() and SIGNAL() */
#define WAIT(s) semop(s, &pop, 1)
#define SIGNAL(s) semop(s, &vop, 1)

void cleanup_and_exit(int);
void cleanup(void);
void err(int);

static message *messages;
static registry_entry *registry;
static int msg_mutex, reg_mutex;
static int msg_shm_id, reg_shm_id;
static struct sembuf pop, vop;

int main(void) {
    key_t k1, k2, k3, k4;

    /* setup sembufs for wait() and signal() */
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1; vop.sem_op = 1;

    /* get keys for usage with semget / shmget */
    printf("Creating keys for shmget() and semget()... ");
    k1 = ftok("/usr/", 1);
    k2 = ftok("/usr/", 2);
    k3 = ftok("/usr/", 3);
    k4 = ftok("/usr/", 4);
    printf("done\n");

    /* create shared memory / semaphores */
    printf("Creating shared memory and semaphores... ");
    err(msg_shm_id = shmget(k1, MAX_MESSAGES * sizeof(message), 0777 | IPC_CREAT));
    err(reg_shm_id = shmget(k2, MAX_PROCESSES * sizeof(registry_entry), 0777 | IPC_CREAT));
    err(msg_mutex = semget(k3, 1, 0777 | IPC_CREAT));
    err(reg_mutex = semget(k4, 1, 0777 | IPC_CREAT));
    printf("done\n");

    /* initialize semaphores */
    printf("Initializing semaphores... ");
    err(semctl(msg_mutex, 0, SETVAL, 1));
    err(semctl(reg_mutex, 0, SETVAL, 1));
    printf("done\n");

    /* attach to shared memory */
    printf("Attaching to shared memory... ");
    err((long) (messages = shmat(msg_shm_id, 0, 0)));
    err((long) (registry = shmat(reg_shm_id, 0, 0)));
    printf("done\n");

    /* bind signal handlers to signals */
    printf("Binding signal handlers to signals... ");
    if (signal(SIGINT, cleanup_and_exit) == SIG_ERR)
        perror("controller");
    if (signal(SIGTERM, cleanup_and_exit) == SIG_ERR)
        perror("controller");
    printf("done\n");


    /* cleanup loop */
    printf("\n");
    while (1) {
        sleep(CLEANUP_INTERVAL);
        cleanup();
    }


    cleanup_and_exit(-1);

    return 0;
}


void cleanup_and_exit(int signal) {
    if (signal == SIGINT)
        printf("\nReceived SIGINT...\n");
    if (signal == SIGTERM)
        printf("\nReceived SIGTERM...\n");

    /* detach from shared memory */
    printf("Detaching from shared memory... ");
    err(shmdt(messages));
    err(shmdt(registry));
    printf("done\n");

    /* mark shared memory / semaphores for removal */
    printf("Removing shared memory and semaphores... ");
    err(shmctl(msg_shm_id, IPC_RMID, 0));
    err(shmctl(reg_shm_id, IPC_RMID, 0));
    err(semctl(msg_mutex, 0, IPC_RMID, 0));
    err(semctl(reg_mutex, 0, IPC_RMID, 0));
    printf("done\n");

    printf("Exiting...\n");

    exit(0);
}

void cleanup(void) {
    int i;
    time_t now = time(NULL);

    printf("Cleaning up old messages (current time: %ld)... ", now);
    for (i = 0; i < MAX_MESSAGES; i++) {
        WAIT(msg_mutex);
        if (messages[i].valid && (now - messages[i].time) >= CLEANUP_AGE)
            messages[i].valid = FALSE;
        SIGNAL(msg_mutex);
    }
    printf("done\n");
}

void err(int val) {
    if (val == -1) {
        perror("controller");
        exit(1);
    }
}
