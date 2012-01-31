#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <error.h>
#include <errno.h>

/* constants */
#define IMAGE_WIDTH_MAX 1024
#define IMAGE_HEIGHT_MAX 768
#define COMPONENTS_PER_PIXEL 3
#define PIXEL_COMPONENT_LEN 3
#define CHARS_PER_PIXEL ((COMPONENTS_PER_PIXEL) * (PIXEL_COMPONENT_LEN) + 3)
#define LINE_LEN_MAX ((IMAGE_WIDTH_MAX) * (CHARS_PER_PIXEL) + 500)
#define MAX_IMAGE_PIXELS ((IMAGE_WIDTH_MAX) * (IMAGE_HEIGHT_MAX))
#define NUM_CHILDREN 4
#define INPUT_MAX 1000
#define ARGS_MAX 4

/* wait() and signal() */
#define WAIT(s) semop(s, &pop, 1)
#define SIGNAL(s) semop(s, &vop, 1)
#define WAIT_N(s, n) do { int i; for (i = 0; i < n; i++) semop(s, &pop, 1); } while (0)
#define SIGNAL_N(s, n) do { int i; for (i = 0; i < n; i++) semop(s, &vop, 1); } while (0)

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

#define ALLOC(type, n) (type *) malloc((n)*sizeof(type))

#define COMMAND(cmd) (strcmp(command[0], cmd) == 0)

#define BRIGHTEN(val, p) (val + (unsigned char) roundf(fabsf(p) * (255 - val)))
#define DARKEN(val, p) (val - (unsigned char) roundf(fabsf(p) * val))
#define BRGT_TRANSFORM(val, p) ((p < 0) ? DARKEN(val, p) : BRIGHTEN(val, p))

/* enumerations */
typedef enum {
    EXIT = 0,
    BRGT = 1
} CommandType;

/* data structures */
typedef struct _pixel {
    unsigned char r;
    unsigned char g;
    unsigned char b;
} pixel;

typedef struct _image {
    unsigned short width;
    unsigned short height;
    pixel pixels[IMAGE_HEIGHT_MAX][IMAGE_WIDTH_MAX];
} image;

typedef struct _control {
    CommandType cmd;
    char *input_file;
    char *output_file;
    unsigned short input_segments[NUM_CHILDREN][2];
    char percent;
} control;

/* external variables */
extern int errno;

/* global variables */
char *prog = "ImageProc";      /* for error handling */

/* semaphores and sembufs */
int parent;                   /* semaphore on which children wait for parent */
int children;                 /* semaphore on which parent waits for children */
struct sembuf pop, vop;       /* sembufs to realize wait() and signal() calls */

/* shared memory segments */
image *input, *output;
control *ctrl;
int shm_ids[100], shm_count = -1;

/* function prototypes */
void image_brgt(void);
void partition_input(void);
void read_input_image(void);
void write_output_image(void);

void chld_brgt(void);
void chld_setup(void);
void chld_exit(void);

void sem_setup(void);
void sem_create(void);
void sem_cleanup(void);

void shm_create(void);
void shm_cleanup(void);

void parse_command(char *[]);
int tokenize_input(char *, char *[]);
void read_input(char *);

void err(int);


int main(void) {
    int num_args;
    char input_str[INPUT_MAX], *command[ARGS_MAX];

    shm_create();

    sem_setup();
    sem_create();

    chld_setup();

    while (1) {
        read_input(input_str);
        if (input_str[0] == '\0')
            continue;
        num_args = tokenize_input(input_str, command);
        parse_command(command);

        if (COMMAND("exit")) {
            chld_exit();
            break;
        }
        else if (COMMAND("brgt")) {
            image_brgt();
        }
    }

    sem_cleanup();
    shm_cleanup();

    return 0;
}

void image_brgt(void) {
    read_input_image();
    partition_input();
    SIGNAL_N(parent, NUM_CHILDREN);
    WAIT_N(children, NUM_CHILDREN);
    printf("%d %d\n", input->width, input->height);
    write_output_image();
}

void partition_input(void) {
    int i;
    float l = (input->height) * 1.0f / NUM_CHILDREN;
    for (i = 0; i < NUM_CHILDREN; i++) {
        ctrl->input_segments[i][0] = (int) (i * l);
        ctrl->input_segments[i][1] = (int) ((i + 1) * l) - 1;
    }
}

void read_input_image(void) {
    int row, col, max_level;
    char signature[3];
    FILE *ifp = fopen(ctrl->input_file, "r");

    /* signature (one of P1 through P6); read and discard */
    fscanf(ifp, "%s", signature);

    /* read image dimensions */
    fscanf(ifp, "%hu %hu", &(input->width), &(input->height));

    /* maximum level; read and discard */
    fscanf(ifp, "%d", &max_level);

    /* read pixel values */
    for (row = 0; row < input->height; row++) {
        for (col = 0; col < input->width; col++) {
            fscanf(ifp, "%hhu %hhu %hhu",
                   &(input->pixels[row][col].r),
                   &(input->pixels[row][col].g),
                   &(input->pixels[row][col].b));
        }
    }

    fclose(ifp);
}

void write_output_image(void) {
    int row, col, max_level = 255;
    char *signature = "P3";
    FILE *ofp = fopen(ctrl->output_file, "w");

    /* signature */
    fprintf(ofp, "%s\n", signature);

    /* image dimensions */
    fprintf(ofp, "%hu %hu\n", output->width, output->height);

    /* maximum level */
    fprintf(ofp, "%d\n", max_level);

    /* write pixel values */
    for (row = 0; row < output->height; row++) {
        for (col = 0; col < output->width; col++) {
            fprintf(ofp, "%hhu %hhu %hhu ",
                    output->pixels[row][col].r,
                    output->pixels[row][col].g,
                    output->pixels[row][col].b);
        }
        fprintf(ofp, "\n");
    }

    fclose(ofp);
}

void chld_brgt(void) {
    int row, col;
    float p = 0.01 * ctrl->percent;
    output->width = input->width;
    output->height = input->height;
    for (row = 0; row < input->height; row++) {
        for (col = 0; col < input->width; col++) {
            output->pixels[row][col].r = BRGT_TRANSFORM(input->pixels[row][col].r, p);
            output->pixels[row][col].g = BRGT_TRANSFORM(input->pixels[row][col].g, p);
            output->pixels[row][col].b = BRGT_TRANSFORM(input->pixels[row][col].b, p);
        }
    }
}

void chld_setup(void) {
    int i;
    for (i = 0; i < NUM_CHILDREN; i++) {
        if (fork() == 0) {
            while (1) {
                WAIT(parent);
                switch (ctrl->cmd) {
                case EXIT:
                    exit(0);
                case BRGT:
                    chld_brgt();
                    break;
                default:
                    break;
                }
                SIGNAL(children);
            }
        }
    }
}

void chld_exit(void) {
    int status;
    SIGNAL_N(parent, NUM_CHILDREN);
    while (wait(&status) > 0)
        ;
}

void sem_setup(void) {
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1; vop.sem_op = 1;
}

void sem_create(void) {
    SEMGET(parent, 0);
    SEMGET(children, 0);
}

void sem_cleanup(void) {
    SEMREM(parent);
    SEMREM(children);
}

void shm_create(void) {
    SHMGET(input, image, 1);
    SHMGET(output, image, 1);
    SHMGET(ctrl, control, 1);
}

void shm_cleanup(void) {
    SHMDT(input);
    SHMDT(output);
    SHMDT(ctrl);
    SHMREM();
}

void parse_command(char *command[]) {
    if (COMMAND("exit")) {
        ctrl->cmd = EXIT;
    }
    else if (COMMAND("brgt")) {
        ctrl->cmd = BRGT;
        ctrl->input_file = command[1];
        ctrl->output_file = command[2];
        ctrl->percent = atoi(command[3]);
    }
}

void read_input(char *input_str) {
    char *prompt = "ImageProc> ", *c;
    printf("%s", prompt);
    fgets(input_str, INPUT_MAX, stdin);
    c = &input_str[strlen(input_str) - 1];
    if (*c == '\n')
        *c = '\0';
}

int tokenize_input(char *input_str, char *command[]) {
    int i = 0;
    command[i++] = strtok(input_str, " ");
    while ((command[i++] = strtok(NULL, " ")) != NULL);
    return i - 1;
}

void err(int ret) {
    if (ret == -1) {
        perror(prog);
        exit(-1);
    }
}
