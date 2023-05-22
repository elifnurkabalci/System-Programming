#include <unistd.h>
#include "biboServer.h"

static char clientFifo[CLIENT_FIFO_NAME_LEN];

int main(int argc, char *argv[]) {

    int serverFd, clientFd;
    struct request req;
    struct response resp;

    if (argc != 3) {
        printf("Usage: %s <connect_type> ServerPID\n", argv[0]);
        return 1;
    }

    int server_pid = atoi(argv[2]);
    int connectType = strcmp(argv[1], "connect") == 0 ? O_WRONLY : O_WRONLY | O_NONBLOCK;

    if (connectType == O_WRONLY)
        printf("Connecting to server (Server ID: %d)\n", server_pid);
    else
        printf("Trying to connect to server (Server ID: %d)\n", server_pid);

    umask(0); 

    snprintf(clientFifo, CLIENT_FIFO_NAME_LEN, CLIENT_FIFO_TEMPLATE, getpid());
    if (mkfifo(clientFifo, S_IRUSR | S_IWUSR | S_IWGRP) == -1 && errno != EEXIST)
        perror("mkfifo");


    req.pid = getpid();

    snprintf(req.seq, sizeof(req.seq), "%s %d", argv[1], server_pid);
    
    serverFd = open(SERVER_FIFO, O_WRONLY);
    if (serverFd == -1)
        perror("open server FIFO");

    if (write(serverFd, &req, sizeof(struct request)) != sizeof(struct request))
        perror("write to server FIFO");


    clientFd = open(clientFifo, O_RDONLY);
    if (clientFd == -1)
        perror("open client FIFO");

    while (1) {
        printf(">> Enter command: ");
        
        if (fgets(req.seq, sizeof(req.seq), stdin) == NULL) {
            printf("EOF or read error occurred\n");
            break;
        }
        req.seq[strcspn(req.seq, "\n")] = '\0';  

        //printf("command: %s\n", req.seq);

        if (strcmp(req.seq, "quit") == 0) {
            break;
        }

        if (write(serverFd, &req, sizeof(struct request)) != sizeof(struct request)){
            perror("Can't write to server");
            continue;
        }
        sleep(1);
        if (read(clientFd, &resp, sizeof(struct response)) != sizeof(struct response)){
            perror("Can't read response from server");
            continue;
        }
        
        printf("Server responded with seqNum %d\n", resp.seqNum);
        
    }
    printf(">> bye\n");

    unlink(clientFifo);
    close(serverFd);
    close(clientFd);

    return 0;
}
