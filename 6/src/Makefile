CCFLAGS = -Wall -Wextra -Werror
DEBUGFLAGS = -g3 -gdwarf-2

.PHONY: tar clean check-syntax

all: SystemInfo ProcessInfo

check-syntax:
	cc $(CCFLAGS) -fsyntax-only SystemInfo.c
	cc $(CCFLAGS) -fsyntax-only ProcessInfo.c

tar:
	tar cvf ../09CS1008.tar SystemInfo.c ProcessInfo.c Makefile

SystemInfo: SystemInfo.c
	cc $(CCFLAGS) $(DEBUGFLAGS) SystemInfo.c -o SystemInfo

ProcessInfo: ProcessInfo.c
	cc $(CCFLAGS) $(DEBUGFLAGS) ProcessInfo.c -o ProcessInfo

clean:
	rm SystemInfo ProcessInfo