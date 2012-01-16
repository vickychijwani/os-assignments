#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
    int status;

    if(!fork()) {
        execlp("./shell", "shell", (char *) NULL);
    }
    wait(&status);

    return 0;
}
