#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define NUM_MAX 100

int linear_search(int, int[], int, int);

int main(void) {
    int i, n, key, nums[NUM_MAX], fd1[2], fd2[2], index, ret1, ret2, status;

    scanf("%d", &n);
    for (i = 0; i < n; i++) {
        scanf("%d", &nums[i]);
    }
    scanf("%d", &key);

    pipe(fd1);
    pipe(fd2);

    /* first child */
    if (!fork()) {
        index = linear_search(key, nums, 0, n/2);
        write(fd1[1], &index, sizeof(index));
    }
    else {
        read(fd1[0], &ret1, sizeof(ret1));

        /* second child */
        if (!fork()) {
            index = linear_search(key, nums, n/2+1, n-1);
            write(fd2[1], &index, sizeof(index));
        }
        else {
            read(fd2[0], &ret2, sizeof(ret2));

            wait(&status);
            wait(&status);

            if (ret1 >= 0)
                printf("The number %d occurs at index %d in the array.\n", key, ret1);
            else if (ret2 >= 0)
                printf("The number %d occurs at index %d in the array.\n", key, ret2);
            else
                printf("The number %d does not occur anywhere in the array.\n", key);
        }
    }

    return 0;
}

int linear_search(int key, int nums[], int start, int end) {
    int i;
    for (i = start; i <= end; i++)
        if (nums[i] == key)
            return i;
    return -1;
}
