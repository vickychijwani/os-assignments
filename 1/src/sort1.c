#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define ALLOC(type,n) (type *) malloc((n)*sizeof(type))

int compar(const void *, const void *);

int main(int argc, char *argv[]) {
    FILE *fp = fopen(argv[1], "r");
    char *str = ALLOC(char, 20);
    int i = 0, j, numbers[1000];

    if (argc != 2) {
        printf("Usage: sort1 <filename>\n");
        exit(1);
    }

    while (fgets(str, 20, fp)) {
        numbers[i++] = atoi(str);
    }

    qsort(&numbers[0], i, sizeof(int), compar);

    for (j = 0; j < i; j++) {
        printf("%d\n", numbers[j]);
    }

    return 0;
}

int compar(const void *a, const void *b) {
    return *(int *) a - *(int *) b;
}
