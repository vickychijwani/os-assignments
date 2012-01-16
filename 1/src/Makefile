CCFLAGS = -Wall -Wextra -pedantic

.PHONY: tar clean

all: run shell xsort sort1

tar:
	tar cvf ../09CS1008.tar Makefile run.c shell.c xsort.c sort1.c

run: shell run.c
	cc $(CCFLAGS) -o ../bin/run run.c

shell: shell.c
	cc $(CCFLAGS) -o ../bin/shell shell.c

xsort: sort1 xsort.c
	cc $(CCFLAGS) -o ../bin/xsort xsort.c

sort1: sort1.c
	cc $(CCFLAGS) -o ../bin/sort1 sort1.c

clean:
	rm ../bin/*