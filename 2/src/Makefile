CCFLAGS = -Wall -Wextra -pedantic
DEBUGFLAGS = -g3 -gdwarf-2

.PHONY: tar clean check-syntax

all: run shell

check-syntax:
	cc $(CCFLAGS) -fsyntax-only shell.c run.c

tar:
	tar cvf ../09CS1008.tar Makefile run.c shell.c

run: shell run.c
	cc $(CCFLAGS) $(DEBUGFLAGS) -o ../bin/run run.c

shell: shell.c
	cc $(CCFLAGS) $(DEBUGFLAGS) -o ../bin/shell shell.c

clean:
	rm ../bin/*