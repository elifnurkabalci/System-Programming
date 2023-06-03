#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <pthread.h>

#include "copy.h"

int numcopiers;
int bufsize;
char destArray[100];
int doneflag = 0;

typedef struct {
    char* src;
    char* dest;
} ThreadData;

void* thread_copy(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    int fd_in, fd_out, bytes_read;
    char* buf;

    if ((buf = (char*)malloc(bufsize)) == NULL) {
        perror("Failed to allocate memory for buffer.");
        pthread_exit(NULL);
    }

    if ((fd_in = open(data->src, O_RDONLY)) == -1) {
        fprintf(stderr, "Failed to open input file %s\n", data->src);
        free(buf);
        pthread_exit(NULL);
    }

    if ((fd_out = open(data->dest, O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1) {
        fprintf(stderr, "Failed to open output file %s\n", data->dest);
        free(buf);
        close(fd_in);
        pthread_exit(NULL);
    }

    while ((bytes_read = read(fd_in, buf, bufsize)) > 0) {
        if (write(fd_out, buf, bytes_read) != bytes_read) {
            fprintf(stderr, "Failed to write to output file %s\n", data->dest);
            free(buf);
            close(fd_in);
            close(fd_out);
            pthread_exit(NULL);
        }
        if (doneflag) {
            printf("Thread interrupted, exiting...\n");
            free(buf);
            close(fd_in);
            close(fd_out);
            pthread_exit(NULL);
        }
    }
    
    if (bytes_read == -1) {
        fprintf(stderr, "Failed to read from input file %s\n", data->src);
    }

    free(buf);
    close(fd_in);
    close(fd_out);
    pthread_exit(NULL);
}

int copy_files(char* inFile, char* outFile) {
    pthread_t* threads;
    ThreadData* data;
    int i, ret;

    if ((threads = (pthread_t*)malloc(numcopiers * sizeof(pthread_t))) == NULL) {
        perror("Failed to allocate memory for threads.");
        return -1;
    }

    if ((data = (ThreadData*)malloc(numcopiers * sizeof(ThreadData))) == NULL) {
        perror("Failed to allocate memory for thread data.");
        free(threads);
        return -1;
    }

    for (i = 0; i < numcopiers; i++) {
        data[i].src = inFile;
        data[i].dest = outFile;
        ret = pthread_create(&threads[i], NULL, thread_copy, &data[i]);
        if (ret != 0) {
            fprintf(stderr, "Failed to create thread %d\n", i);
            free(data);
            free(threads);
            return -1;
        }
    }

    for (i = 0; i < numcopiers; i++) {
        ret = pthread_join(threads[i], NULL);
        printf("Thread %d copied %d bytes from source to destionation\n", (i+1), bufsize);
        if (ret != 0) {
            fprintf(stderr, "Failed to join thread %d\n", i);
            free(data);
            free(threads);
            return -1;
        }
    }

    free(data);
    free(threads);
    return 0;
}
