#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h> 

void with_x(char* filename, int num_bytes, char* x, mode_t mode, int fd){
    int i;
    off_t offset; // lseek control
    
    for (i = 0; i < num_bytes; i++) {
        offset = lseek(fd, 0, SEEK_END); // seek to end of file
        if (offset == -1) {
            perror("lseek");
            exit(EXIT_FAILURE);
        }

        if (write(fd, x, 1) == -1) { // write one byte to the file
            perror("write");
            exit(EXIT_FAILURE);
        }
    }
}
void without_x(char* filename, int num_bytes, char* x, mode_t mode, int fd){
    int i;
    for (i = 0; i < num_bytes; i++) {
        if (write(fd, x, 1) == -1) { // write one byte to the file
            perror("write");
            exit(1);
        }
    }
}
void part1(int argc, char *argv[]){
    int num_bytes, fd;
    char *filename, *x, *a; 
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; // set file mode

    if (argc < 3) { // error handling
        fprintf(stderr, "Usage: %s filename num-bytes [x]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    filename = argv[1];
    num_bytes = strtol(argv[2], NULL, 10);
    x = "a";
    a = argv[argc-1];
    if(argc == 3){ // without x
        fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, mode); // open file with O_APPEND flag
        if (fd == -1) { // control
            perror("open");
            exit(EXIT_FAILURE);
        }
        without_x(filename, num_bytes, x, mode,fd);
    }
    else{ // with x
        fd = open(filename, O_WRONLY | O_CREAT, mode); // open file without O_APPEND flag
        if (fd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        with_x(filename, num_bytes, x, mode,fd);
    }
    if (close(fd) == -1) { // close the file
        perror("close");
        exit(EXIT_FAILURE);
    }

}
int my_dup(int oldfd){
    mode_t mode2 = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; // set file mode for dublicated new file
    int newfd = fcntl(oldfd, F_DUPFD, 0);
    if (newfd == -1) { // if file cannot open return -1
        return -1;
    } else {
        newfd = open("generated_dup_file", O_WRONLY | O_CREAT | O_TRUNC, mode2); // new file
        char buf[BUFSIZ];
        ssize_t num_read;
        while ((num_read = read(oldfd, buf, BUFSIZ)) > 0) { // read from old file, write to new file.
            ssize_t num_written = write(newfd, buf, num_read);
        }
        return newfd;
    }
}
int my_dup2(int oldfd, int newfd) {
    if (oldfd == newfd) {
        int flags = fcntl(oldfd, F_GETFL, 0); // check oldfd valid or not
        // F_GETFL is get flag for a file descriptor
        if (flags == -1) {
            errno = EBADF; // set of EBADF
            return errno; // return -1
        } else {
            return oldfd; 
        }
    } else {
        int result = fcntl(oldfd, F_DUPFD, newfd); // duplicate a file descriptor
        if (result == -1) { // if file cannot dublicated return -1 
            return -1;
        } else {
            char buf[BUFSIZ]; // general declaration of buffer size
            ssize_t num_read; 
            while ((num_read = read(oldfd, buf, BUFSIZ)) > 0) { // read from old file, write to newfile
                ssize_t num_written = write(newfd, buf, num_read);
            }
            return result;
        }
    }
}
void compare(int fd1, int fd2){
    off_t offset1, offset2;
    // Set the file offset for the first file descriptor
    offset1 = lseek(fd1, 0, SEEK_CUR);
    
    // Get the file offset for the second file descriptor
    offset2 = lseek(fd2, 0, SEEK_CUR);

    if (offset1 == offset2) {
        printf("File offset values are the same\n");
    } else {
        printf("File offset values are different\n");
    }

}
void part3_for_dub(char* filename){
    int fd1, fd2;
    mode_t mode2 = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; // set file mode for dublicated new file
    mode_t mode = S_IRUSR | S_IRGRP | S_IROTH; // set file mode

    // open file for idenitical the fd1 is filename, we use this
    fd1 = open(filename, O_RDONLY, mode); 

    // dublicate the fd1 to fd2 with dub
    fd2 = my_dup(fd1);

    compare(fd1,fd2);    

    if (close(fd1) == -1) { // close the file
        perror("close");
        exit(EXIT_FAILURE);
    }
    if (close(fd2) == -1) { // close the file
        perror("close");
        exit(EXIT_FAILURE);
    }
}
void  part3_for_dub2(char* filename){
    int fd1, fd2;
    off_t offset1, offset2;
    mode_t mode1 = S_IRUSR | S_IRGRP | S_IROTH; // set file mode, for reading only
    mode_t mode2 = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; // set file mode for duplicated new file

    // open files
    fd1 = open(filename, O_RDONLY, mode1); 
    fd2 = open("generated_dup2_file", O_WRONLY | O_CREAT | O_TRUNC, mode2); // new file

    // dublicate the fd1 to fd2 with dub2
    int result = my_dup2(fd1, fd2);
    if(result != -1) printf("Success in dub2.\n");
    // compare with f1 and result, because of dup2's result is returned duplicated file.
    compare(fd1, result);
    
    if (close(fd2) == -1) { // close the file
        perror("close");
        exit(EXIT_FAILURE);
    }
    if (close(fd1) == -1) { // close the file
        perror("close");
        exit(EXIT_FAILURE);
    }
    
}
void part2_3(char* filename){
    perror("Test for dub");
    part3_for_dub(filename); // test for dup

    perror("Test for dub2"); 
    part3_for_dub2(filename); // test for dup2

}
void main(int argc, char *argv[]) {
    part1(argc,argv);
    part2_3(argv[1]);
        
    exit(EXIT_SUCCESS);
}
