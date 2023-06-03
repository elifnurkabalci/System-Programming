all: pCp

pCp: main.o copy.o directory.o -lpthread
	gcc main.o copy.o directory.o -lpthread -o pCp

pCp.o: pCp.o
	gcc -c pCp.c

main.o: main.c directory.h copy.h -lpthread
	gcc -c main.c -lpthread

copy.o: copy.c copy.h -lpthread
	gcc -c copy.c -lpthread

directory.o: directory.c directory.h copy.h -lpthread
	gcc -c directory.c -lpthread


clean:
	rm -f *.o pCp
