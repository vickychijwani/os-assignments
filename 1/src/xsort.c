#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    int status;

    if (argc != 2) {
        printf("Usage: xsort <filename>\n");
        exit(1);
    }

    if (!fork()) {
        execlp("xterm", "xterm", "-hold", "-e", "./sort1", argv[1], (char *) NULL);
    }
    wait(&status);

    return 0;
}
