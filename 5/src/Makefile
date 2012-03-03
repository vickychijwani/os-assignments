CCFLAGS = -Wall -Wextra -Werror -Wno-format-zero-length -lpthread
DEBUGFLAGS = -g3 -gdwarf-2

.PHONY: tar clean check-syntax

all: BoolMat

check-syntax: BoolMat.c
	cc $(CCFLAGS) -fsyntax-only BoolMat.c

tar:
	tar cvf ../09CS1008.tar BoolMat.c Makefile

BoolMat: BoolMat.c
	cc $(CCFLAGS) $(DEBUGFLAGS) BoolMat.c -o BoolMat

clean:
	rm BoolMat