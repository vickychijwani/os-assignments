CCFLAGS = -Wall -Wextra -Werror
DEBUGFLAGS = -g3 -gdwarf-2

.PHONY: tar clean check-syntax

lab: mympl.h mympl.c controller.c msgclient.c
	cc -c $(DEBUGFLAGS) mympl.c
	ar rcs libmympl.a mympl.o
	rm mympl.o
	cc $(CCFLAGS) $(DEBUGFLAGS) controller.c -L. -lmympl -o controller
	cc $(CCFLAGS) $(DEBUGFLAGS) msgclient.c -L. -lmympl -o msgclient

all: lib controller msgclient

check-syntax:
	cc $(CCFLAGS) -fsyntax-only mympl.c
	cc $(CCFLAGS) -fsyntax-only msgclient.c
	cc $(CCFLAGS) -fsyntax-only controller.c

tar:
	tar cvf ../09CS1008.tar mympl.h mympl.c controller.c msgclient.c Makefile

lib: mympl.c mympl.h
	cc -c $(DEBUGFLAGS) mympl.c
	ar rcs ../bin/libmympl.a mympl.o
	rm mympl.o

controller: controller.c lib
	cc $(CCFLAGS) $(DEBUGFLAGS) controller.c -L../bin -lmympl -o ../bin/controller

msgclient: msgclient.c lib
	cc $(CCFLAGS) $(DEBUGFLAGS) msgclient.c -L../bin -lmympl -o ../bin/msgclient

clean:
	rm libmympl.a controller msgclient

bin-clean:
	rm ../bin/*