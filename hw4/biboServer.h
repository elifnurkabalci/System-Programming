#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "tlpi_hdr.h"

#define SERVER_FIFO "/tmp/bibo_sv"
#define CLIENT_FIFO_TEMPLATE "/tmp/bibo_cl.%d"
#define CLIENT_FIFO_NAME_LEN (sizeof(CLIENT_FIFO_TEMPLATE) + 20)


struct request{ // client-> server
	pid_t pid;
	int seqLen;
	char seq[1024];
};

struct response{ //server -> client
	int seqNum; // start of sequence
	char message[1024];
};

struct client_data{
    int fd;
    struct request req;
};

struct shared_memory {
    int client_pid;
    char command[MAX_FILE_SIZE];
    int response_status;
    char response_message[MAX_FILE_SIZE];
};