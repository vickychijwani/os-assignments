CCFLAGS = -Wall -Wextra -Werror -lm -O0
DEBUGFLAGS = -g3 -gdwarf-2

.PHONY: tar clean check-syntax

all: lib

check-syntax:
	cc $(CCFLAGS) -lm -fsyntax-only list.c mem_mgmt.c mem_test.c sort_merge.c

tar:
	tar cvf ../09CS1008.tar Makefile mem_mgmt.h mem_mgmt.c mem_test.c sort_merge.c

lib: mem_mgmt.c mem_mgmt.h list
	cc -c $(CCFLAGS) $(DEBUGFLAGS) mem_mgmt.c
	ar rcs libmem_mgmt.a mem_mgmt.o list.o
	rm mem_mgmt.o

list: list.c list.h
	cc -c $(CCFLAGS) $(DEBUGFLAGS) list.c

test: test.c lib
	cc -lm $(DEBUGFLAGS) test.c -L. -lmem_mgmt -o test

clean:
	rm libmem_mgmt.a *.o test