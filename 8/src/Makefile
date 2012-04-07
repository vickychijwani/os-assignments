CCFLAGS = -Wall -Wextra -lm
DEBUGFLAGS = -g3 -gdwarf-2

.PHONY: tar clean check-syntax

all: format fuse

format: format.c
	cc $(CCFLAGS) $(DEBUGFLAGS) -o format format.c

fuse: os-fs.o params.h fs_functions.o logger.o
	cc $(CCFLAGS) $(DEBUGFLAGS) -o os-fs os-fs.o fs_functions.o logger.o `pkg-config fuse --cflags --libs` -lm

fs_functions.o: fs_functions.c config.h structs.h fs_functions.h macros.h
	cc $(CCFLAGS) $(DEBUGFLAGS) -lm -c fs_functions.c `pkg-config fuse --cflags --libs` -lm

logger.o: logger.c logger.h
	cc $(CCFLAGS) $(DEBUGFLAGS) -c logger.c `pkg-config fuse --cflags --libs`

os-fs.o: os-fs.c params.h
	cc $(CCFLAGS) $(DEBUGFLAGS) -c os-fs.c `pkg-config fuse --cflags --libs` -lm

test: test.c fs_functions.o logger.o
	cc $(CCFLAGS) $(DEBUGFLAGS) -o test test.c fs_functions.o logger.o -lm

check-syntax:
	cc $(CCFLAGS) -fsyntax-only fs_functions.c os-fs.c

tar:
	tar cvf ../09CS1008.tar fs_functions.h fs_functions.c logger.h logger.c params.h config.h structs.h macros.h format.c os-fs.c Makefile

clean:
	rm format os-fs *.o
