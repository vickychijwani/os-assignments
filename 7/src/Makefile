CCFLAGS = -Wall -Wextra -Werror
DEBUGFLAGS = -g3 -gdwarf-2

.PHONY: tar clean check-syntax

all: lib mem_test sort_merge

check-syntax:
	cc $(CCFLAGS) -lm -fsyntax-only list.c mem_mgmt.c mem_test.c sort_merge.c

tar:
	tar cvf ../09CS1008.tar Makefile mem_mgmt.h mem_mgmt.c list.h list.c mem_test.c sort_merge.c

lib: mem_mgmt.c mem_mgmt.h list
	cc -c $(CCFLAGS) $(DEBUGFLAGS) mem_mgmt.c
	ar rcs libmem_mgmt.a mem_mgmt.o list.o
	rm mem_mgmt.o

list: list.c list.h
	cc -c $(CCFLAGS) $(DEBUGFLAGS) list.c

mem_test: mem_test.c lib
	cc -lm $(DEBUGFLAGS) mem_test.c -L. -lmem_mgmt -o mem_test

sort_merge: sort_merge.c lib
	cc -lm $(DEBUGFLAGS) sort_merge.c -lpthread -L. -lmem_mgmt -o sort_merge

clean:
	rm libmem_mgmt.a *.o mem_test sort_merge