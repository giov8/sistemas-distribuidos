all: vcube

vcube: vcube.o smpl.o rand.o token_agrupada.o
	$(LINK.c) -o $@ -Bstatic vcube.o smpl.o rand.o token_agrupada.o -lm

smpl.o: smpl.c smpl.h
	$(COMPILE.c)  -g smpl.c

vcube.o: vcube.c smpl.h
	$(COMPILE.c) -g  vcube.c

rand.o: rand.c
	$(COMPILE.c) -g rand.c

token_agrupada.o: token_agrupada.c
	$(COMPILE.c) -g token_agrupada.c

clean:
	$(RM) *.o vcube relat saida

