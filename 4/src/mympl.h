#ifndef _MYMPL_H_
#define _MYMPL_H_

/* constants */
#define TRUE 1
#define FALSE 0

#define MAX_NAME_LEN 10
#define MAX_MSG_LEN 100
#define MAX_TIME_LEN 15

#define MAX_PROCESSES 10
#define MAX_MESSAGES 100

/* error constants */
#define SUCCESS 1

#define REG_FULL -1
#define REG_ALRDYREG -2
#define REG_FAIL -3

#define DEREG_FAIL -1

#define LIST_FAIL -1

#define READ_NOMSG 0
#define READ_FAIL -1

#define SEND_NOTREGD -1
#define SEND_NOSLOTS -2
#define SEND_FAIL -3

/* data structures */
typedef struct {
    char sender[MAX_NAME_LEN + 1];
    char recipient[MAX_NAME_LEN + 1];
    time_t time;
    char text[MAX_MSG_LEN + 1];
    char valid;
} message;

typedef struct {
    char name[MAX_NAME_LEN + 1];
    pid_t pid;
    char valid;
} registry_entry;

/* extern union semun; */
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short  *array;
};

/* API calls */
/* NOTE: THIS FUNCTION IS NAMED "reg" BECAUSE "register" IS A KEYWORD */
int reg(char *procname);
int deregister(void);

int listmessages(void);
int readmessage(message *buffer);
int sendmessage(char *recptname, char *textmsg);

#endif /* _MYMPL_H_ */
