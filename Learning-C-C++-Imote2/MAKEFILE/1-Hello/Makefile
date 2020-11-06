all: hello

hello: hellomake.o hellofunc.o
	gcc -Wall hellomake.o hellofunc.o -o hello

hellomake.o: hellomake.c
	gcc -Wall -c hellomake.c

hellofunc.o: hellofunc.c
	gcc -Wall -c hellofunc.c 
