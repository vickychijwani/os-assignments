CCFLAGS = -Wall -Wextra -pedantic
DEBUGFLAGS = -g3 -gdwarf-2
SHELL_TESTS = assign1 cp
SEARCH_TESTS = search_both_halves search_first_half search_second_half search_no_half

.PHONY: tar clean check-syntax

all: run shell search

check-syntax:
	cc $(CCFLAGS) -fsyntax-only shell.c run.c

tar:
	tar cvf ../09CS1008.tar Makefile run.c shell.c search.c

test: all
	cd ../tests/; \
	for test in $(SHELL_TESTS); do \
		../bin/shell < in_$$test > /tmp/out_$$test; \
		colordiff -u out_$$test /tmp/out_$$test; \
		rm /tmp/out_$$test; \
	done; \
	for test in $(SEARCH_TESTS); do \
		../bin/search < in_$$test > /tmp/out_$$test; \
		colordiff -u out_$$test /tmp/out_$$test; \
		rm /tmp/out_$$test; \
	done

search: search.c
	cc $(CCFLAGS) $(DEBUGFLAGS) -o ../bin/search search.c

run: shell run.c
	cc $(CCFLAGS) $(DEBUGFLAGS) -o ../bin/run run.c

shell: shell.c
	cc $(CCFLAGS) $(DEBUGFLAGS) -o ../bin/shell shell.c

lab:
	cc $(CCFLAGS) -o search search.c
	cc $(CCFLAGS) -o shell shell.c
	cc $(CCFLAGS) -o run run.c

lab-clean:
	rm search shell run

clean:
	rm ../bin/*