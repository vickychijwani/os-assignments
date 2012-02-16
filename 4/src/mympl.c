#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <errno.h>

#include "mympl.h"


/* macros for convenience : WAIT() and SIGNAL() */
#define WAIT(s) semop(s, &pop, 1)
#define SIGNAL(s) semop(s, &vop, 1)


/* misc function prototypes */
void sem_setup(void);


/* book-keeping variables */
static int registered = FALSE;
static message *messages;
static registry_entry *registry;
static int msg_mutex, reg_mutex;
static char *name;
static struct sembuf pop, vop;


/* implementations of API calls */
int reg(char *procname) {
    int i, val;
    int msg_shm_id, reg_shm_id;
    key_t k1, k2, k3, k4;

    /* test procname */
    if (procname == NULL || strlen(procname) > MAX_NAME_LEN)
        return REG_FAIL;

    /* setup sembufs for wait() and signal() */
    sem_setup();

    /* get keys for usage with semget / shmget */
    k1 = ftok("/usr/", 1);
    k2 = ftok("/usr/", 2);
    k3 = ftok("/usr/", 3);
    k4 = ftok("/usr/", 4);

    /* get existing shared memory segments / semaphores */
    if ((msg_shm_id = shmget(k1, 0, 0)) == -1) {
        perror("mympl");
        return REG_FAIL;
    }
    if ((reg_shm_id = shmget(k2, 0, 0)) == -1) {
        perror("mympl");
        return REG_FAIL;
    }
    if ((msg_mutex = semget(k3, 0, 0)) == -1) {
        perror("mympl");
        return REG_FAIL;
    }
    if ((reg_mutex = semget(k4, 0, 0)) == -1) {
        perror("mympl");
        return REG_FAIL;
    }

    /* busy-wait till semaphores get initialized by the controller (to prevent race condition) */
    while ((val = semctl(msg_mutex, 0, GETVAL)) != 1) {
        if (val == -1) {
            perror("mympl");
            return REG_FAIL;
        }
    }
    while ((val = semctl(reg_mutex, 0, GETVAL)) != 1) {
        if (val == -1) {
            perror("mympl");
            return REG_FAIL;
        }
    }

    /* attach to the shared memory segments */
    if ((long) (messages = shmat(msg_shm_id, 0, 0)) == -1) {
        perror("mympl");
        return REG_FAIL;
    }
    if ((long) (registry = shmat(reg_shm_id, 0, 0)) == -1) {
        perror("mympl");
        return REG_FAIL;
    }

    /* if the *CURRENT* process is already registered, or if ANOTHER process has taken the same name, then fail ... */
    for (i = 0; i < MAX_PROCESSES; i++) {
        WAIT(reg_mutex);
        if (registry[i].valid && registry[i].pid == getpid()) {
            SIGNAL(reg_mutex);
            return REG_FAIL;
        }
        else if (registry[i].valid && strcmp(registry[i].name, procname) == 0) {
            SIGNAL(reg_mutex);
            return REG_ALRDYREG;
        }
        SIGNAL(reg_mutex);
    }

    /* ... otherwise, register it */
    for (i = 0; i < MAX_PROCESSES; i++) {
        WAIT(reg_mutex);
        if (! (registry[i].valid)) {
            strcpy(registry[i].name, procname);
            name = registry[i].name;
            registry[i].pid = getpid();
            registry[i].valid = TRUE;
            registered = TRUE;
            SIGNAL(reg_mutex);
            return SUCCESS;
        }
        SIGNAL(reg_mutex);
    }

    /* if the process is still not registered, it means the registry is full */
    return REG_FULL;
}



int deregister(void) {
    int i;

    for (i = 0; i < MAX_PROCESSES; i++) {
        WAIT(reg_mutex);
        if (registry[i].valid && registry[i].pid == getpid()) {
            registry[i].valid = FALSE;
            SIGNAL(reg_mutex);
            return SUCCESS;
        }
        SIGNAL(reg_mutex);
    }

    if (shmdt(messages) == -1) {
        perror("mympl");
        return DEREG_FAIL;
    }
    if (shmdt(registry) == -1) {
        perror("mympl");
        return DEREG_FAIL;
    }

    return DEREG_FAIL;
}



int listmessages(void) {
    int i, count = 0;
    char time_str[MAX_TIME_LEN + 1];

    if (! registered) return LIST_FAIL;

    for (i = 0; i < MAX_MESSAGES; i++) {
        WAIT(msg_mutex);
        if (messages[i].valid && strcmp(messages[i].recipient, name) == 0) {
            count++;
            strftime(time_str, MAX_TIME_LEN, "%d %b, %H:%M", localtime(&(messages[i].time)));
            printf("\n%3d) From: %s\n     Sent: %s\n", count, messages[i].sender, time_str);
        }
        SIGNAL(msg_mutex);
    }

    return count;
}



int readmessage(message *buffer) {
    int i;

    if (buffer == NULL) return READ_FAIL;

    buffer->valid = FALSE;
    for (i = 0; i < MAX_MESSAGES; i++) {
        WAIT(msg_mutex);
        if (messages[i].valid && strcmp(messages[i].recipient, name) == 0) {
            if (! (buffer->valid) || buffer->time > messages[i].time) {
                strcpy(buffer->sender, messages[i].sender);
                strcpy(buffer->recipient, messages[i].recipient);
                buffer->time = messages[i].time;
                strcpy(buffer->text, messages[i].text);
                buffer->valid = TRUE;
                messages[i].valid = FALSE;
            }
        }
        SIGNAL(msg_mutex);
    }

    if (buffer->valid) return SUCCESS;
    else return READ_NOMSG;
}



int sendmessage(char *recptname, char *textmsg) {
    int i;

    if (! registered) return SEND_NOTREGD;
    if (recptname == NULL || strlen(recptname) > MAX_NAME_LEN) return SEND_FAIL;
    if (textmsg == NULL || strlen(textmsg) > MAX_MSG_LEN) return SEND_FAIL;

    for (i = 0; i < MAX_MESSAGES; i++) {
        WAIT(msg_mutex);
        if (! (messages[i].valid)) {
            strcpy(messages[i].sender, name);
            strcpy(messages[i].recipient, recptname);
            messages[i].time = time(NULL);
            strcpy(messages[i].text, textmsg);
            messages[i].valid = TRUE;
            SIGNAL(msg_mutex);
            return SUCCESS;
        }
        SIGNAL(msg_mutex);
    }

    return SEND_NOSLOTS;
}



/* misc functions */
void sem_setup(void) {
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1; vop.sem_op = 1;
}
