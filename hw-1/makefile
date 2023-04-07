all: appendMeMore

appendMeMore: appendMeMore.o
    gcc appendMeMore.o -o appendMeMore

appendMeMore.o: hw1.c 
    gcc -c -Wall -pendantic-errors appendMeMore.c

clean:
    rm -rf * appendMeMore