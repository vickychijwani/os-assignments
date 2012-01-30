CCFLAGS = -Wall -Wextra -pedantic
DEBUGFLAGS = -g3 -gdwarf-2

.PHONY: tar clean check-syntax

all: ProdCons ImageProc

check-syntax:
	cc $(CCFLAGS) -fsyntax-only ProdCons.c
	cc $(CCFLAGS) -fsyntax-only ImageProc.c

tar:
	tar cvf ../09CS1008.tar ProdCons.c ImageProc.c

ProdCons: ProdCons.c
	cc $(CCFLAGS) $(DEBUGFLAGS) -o ../bin/ProdCons ProdCons.c

ImageProc: ImageProc.c
	cc $(CCFLAGS) $(DEBUGFLAGS) -o ../bin/ImageProc ImageProc.c

lab: ProdCons.c ImageProc.c
	cc $(CCFLAGS) -o ProdCons ProdCons.c
	cc $(CCFLAGS) -o ImageProc ImageProc.c

lab-clean:
	rm ProdCons ImageProc

clean:
	rm ../bin/*