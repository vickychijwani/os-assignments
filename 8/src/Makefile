CCFLAGS = -Wall -Wextra -Werror
DEBUGFLAGS = -g3 -gdwarf-2

.PHONY: tar clean check-syntax

all:

check-syntax:
	cc $(CCFLAGS) -fsyntax-only fs_functions.c

tar:
	tar cvf ../09CS1008.tar Makefile config.h structs.h fs_functions.c fs_functions.h

clean: