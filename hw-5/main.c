#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/stat.h>

#include "directory.h"
#include "copy.h"

#define MILLION 1000000L

void sighandler(int signo) { // signal handler for ctrl-c
    printf("SIGINT(C)\n");
    doneflag = 1;
}

int main(int argc, char *argv[]) {
    long timedif; // for calculation time for running
    struct timeval tpend;
    struct timeval tpstart;
    
    if (argc != 5) { // number of argv control
        fprintf(stderr, "Usage: %s infile outfile copies\n", argv[0]);
        return 1;
    }
    // assing user inputs 
    numcopiers = atoi(argv[1]);
    bufsize = atoi(argv[2]);
    strcpy(destArray, argv[4]);
    

    //check the source and destination folders.
    struct stat st = {0};
    if (stat(argv[3], &st) == -1) {
        printf("Source folder %s does not exist!\n", argv[3]);
        exit(EXIT_FAILURE);
    }

    if (stat(destArray, &st) == -1) {
        printf("Destination folder %s does not exist! But it is created..\n", destArray);
        mkdir(destArray, 0700);
    }// if there is no such a thing destination folder, create it.


    if (gettimeofday(&tpstart, NULL)) {
        fprintf(stderr, "Failed to get start time\n");
        return 1;
    }// start and end timer with gettimeofday
    
    struct sigaction sigact;
    sigemptyset(&(sigact.sa_mask)); // signal 
    sigact.sa_handler = sighandler;
    sigaction(SIGINT, &sigact, NULL);
    
    directory(argv[3]); 
    
    if (gettimeofday(&tpend, NULL)) {
        fprintf(stderr, "Failed to get end time\n");
        return 1;
    }
    
    timedif = MILLION * (tpend.tv_sec - tpstart.tv_sec) + tpend.tv_usec - tpstart.tv_usec;
    printf("The function_to_time took %ld microseconds\n", timedif);
    
    return 0;
}
