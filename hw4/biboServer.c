#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include "tlpi_hdr.h"
#include "biboServer.h"
#include <time.h>


void sig_handler(int signum) {
    printf("Received signal %d\n", signum); // normal signal handle
}
void sigkill_handler(int signum) {
    printf("SIGKILL received. Exiting...\n"); // kill signal handle
    exit(1);
}
void *handle_request(void *data, int clientFd, char clientFifo[CLIENT_FIFO_NAME_LEN], struct response resp){
    struct client_data *clientData = (struct client_data *)data;

    char *command = clientData->req.seq;
    //struct response resp; // cikarılabilir
    int test = 0;
    char *command_name = strtok(command, " ");
    char *first_arg = strtok(NULL, " ");
    char *second_arg = strtok(NULL, " ");
    char *str = strtok(NULL, "");

    if(strcmp(command_name, "tryconnect") == 0 || strcmp(command_name, "connect") == 0){

    }
    else if (strcmp(command_name, "killServer") == 0){
        printf("exit by %d\n", clientFd);
        sigkill_handler(SIGINT);
    }
    else if(strcmp(command_name, "help") == 0){
        printf("help\n");
        printf("List of possible client requests:\n- list: Display the list of files\n");
        printf("- readF: Read contents of a file\n- writeT: Write contents to a file\n");
        printf("- upload: Upload a file to the server\n- download: Download a file from the server\n");
        printf("- help: Display the list of possible client requests\n");
    }
    else if (strcmp(command_name, "list") == 0) {
        printf("list\n");

        DIR *dir = opendir(".");
        if (dir == NULL) {
            perror("opendir");
            test = 1;
        } 
        else{
            snprintf(resp.message, sizeof(resp.message), "List of files:\n");
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if (entry->d_type == DT_REG) { // Only include regular files
                    snprintf(resp.message + strlen(resp.message), sizeof(resp.message) - strlen(resp.message), "%s\n", entry->d_name);
                }
            }
            printf("%s\n", resp.message);

            closedir(dir);
        }
    }

    else if (strcmp(command_name, "readF") == 0){
        printf("readF\n");
    }
    else if (strcmp(command_name, "writeT") == 0){
        printf("writeT\n");
    }
    else if (strcmp(command_name, "upload") == 0){
        printf("upload\n");
    }
    else if (strcmp(command_name, "download") == 0){
        printf("download\n");
    }
    else if(strcmp(command_name, "quit") == 0){
        printf("quit\n");
    }
    else{
        printf("Unknown command\n");
        test = 1;
    }
    if(test == 0){
        if (write(clientFd, &resp, sizeof(struct response)) != sizeof(struct response))
            perror("write to client FIFO");
    }
    close(clientFd); 
    return NULL;
}
void log_file(pid_t pid, char *command){
    time_t t;
    t = time(NULL);
    char log_filename[256];
    strftime(log_filename, sizeof(log_filename), "%Y%m%d%H%M%S.log", localtime(&t));
    FILE *log_file = fopen(log_filename, "a");
    fprintf(log_file, "pid: %d, command: %s\n", pid, command);
    fclose(log_file);
}
int main(int argc, char *argv[]){
    //signal(SIGINT, sig_handler);
    //signal(SIGTERM, sig_handler);
    //signal(SIGKILL, sigkill_handler);

    int serverFd, dummyFd, clientFd;
    char clientFifo[CLIENT_FIFO_NAME_LEN];
    struct request req;
    struct response resp;
    int max_clients;

    int num_clients = 0;
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <directory> <num_clients>\n", argv[0]);
        return 1;
    }

    mkdir(argv[1], 0777);
    if (chdir(argv[1]) == -1){
        perror("chdir");
        exit(1);
    }

    max_clients = atoi(argv[2]);
    pid_t child_pids[max_clients];

    umask(0);

    if (mkfifo(SERVER_FIFO, S_IRUSR | S_IWUSR | S_IWGRP) == -1 && errno != EEXIST){
        perror("mkfifo");
        exit(1);
    }

    serverFd = open(SERVER_FIFO, O_RDONLY);
    if (serverFd == -1){
        perror("open");
        exit(1);
    }

    while(1){
        if (read(serverFd, &req, sizeof(struct request)) != sizeof(struct request)){
            //fprintf(stderr, "Error reading request; discarding\n");
            continue;
        }

        snprintf(clientFifo, CLIENT_FIFO_NAME_LEN, CLIENT_FIFO_TEMPLATE, req.pid);

        clientFd = open(clientFifo, O_WRONLY);
        if (clientFd == -1) {
            perror("open");
            continue;
        }

         pid_t pid = fork();
         //printf("%d\n", pid);
        if (pid < 0){
            perror("fork");
            continue;
        }
        else if (pid == 0){ // child process
            close(serverFd); 
            struct client_data data = {.fd = clientFd, .req = req};
            handle_request(&data, data.fd, clientFifo, resp);
            exit(0);
        }
        else{ // parent process
            close(clientFd);
            child_pids[num_clients++] = pid;
        }
        //log_file(getpid(), req.seq);
        int status;
        waitpid(pid, &status, 0);
    }
    unlink(SERVER_FIFO);

    return 0;
}