#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include  "fs_functions.h"
#include <math.h>
//extern void errorExit(char * errorStr);

int main()
{
    /* Format the disk */
    printf("Formatting Disk....");
    myformat("fs");
    printf("Done\n");

    /* Mount the disk */
    printf("Mounting Disk ....");
    if(mount("fs") == 0)
        printf("Done.\n");
    else {
        fprintf(stderr,"Error mounting disk\n");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < n; i++) {
        mymkdir("/");
    }

    unmount("fs");

    return 0;
}
