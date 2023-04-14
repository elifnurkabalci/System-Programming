#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

void sig_handler(int signum) {
    printf("\nReceived signal %d\n", signum); // normal signal handle
}
void sigkill_handler(int signum) {
    printf("\nSIGKILL received. Exiting...\n"); // kill signal handle
    exit(0);
}
void parsing(char* shell[20], int* size, char line[1024]){
	char *split = strtok(line, "|\n"); // | or \n or " " can be split the line
	int temp=0;
	while(split && *size < 20){ // if size is under the 20 and split is not null
		shell[(*size)++] = split; 
		split = strtok(NULL, "|\n");
	}
} 
void file_func(int pipe_fd[2], int *out_fd){ // writing to file byte-byte
	char buffer[4096];
    ssize_t bytes_read;
    while ((bytes_read = read(pipe_fd[0], buffer, sizeof(buffer))) > 0) {
        write(*out_fd, buffer, bytes_read);
    }
}
void wait_status(pid_t pid){ // for waitpid
	int status;
	pid_t wpid = waitpid(pid, &status, 0); 
	if (wpid == -1) {
	perror("waitpid");
	exit(1);
	}
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

void create_pipeline(char *command, int *in_fd, int *out_fd){
	// pipe declaration
	int pipe_fd[2];

	if (pipe(pipe_fd) == -1) {
        perror("pipe");
        exit(1);
    }
    // process child creation
    int pid = fork();
    if(pid==0){
    	if (strncmp(command, "cd", 2) == 0) { // execl function cannot handle cd command
	        char *path = strtok((command+2), " \t\n");
    		if (chdir(path) != 0) {
        		perror("chdir");
    		}
	        return;
	    }
	    if(*in_fd != 0){
	    	dup2(*in_fd, 0);
	    	close(*in_fd);
	    }
	    dup2(pipe_fd[1], 1);
	    close(pipe_fd[0]); close(pipe_fd[1]);
	    execl("/bin/sh", "sh", "-c", command, NULL);
	    perror("exec command");
	    exit(1);
    }else if (pid < 0) {
        perror("child creation");
        exit(1);
    }
    close(pipe_fd[1]);

    if(*out_fd == 1){
    	wait_status(pid);
    	file_func(pipe_fd, out_fd); // for writing
    }else{
    	dup2(pipe_fd[0], *out_fd);
    }
    close(pipe_fd[0]);
    log_file(pid, command);
}
void run_func(char* shell[20], int size){
	int in_fd = 0, out_fd = 1, i=0;
    for(; i<size; ++i){
    	create_pipeline(shell[i], &in_fd, &out_fd);
    }
}
void main(){
	signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGKILL, sigkill_handler);
    while(1){
    	char *shell[20];
    	char line[1024];
    	int size = 0;

    	printf("myshell> ");
    	fgets(line, sizeof(line), stdin);
    	if(strcmp(line, ":q\n") == 0){ 
    		// if user enter :q or SIGKILL, program will exit
    		exit(0);
    	}
    	parsing(shell, &size, line);
    	run_func(shell, size);
    }
}