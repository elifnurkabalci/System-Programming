#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include "directory.h"
#include "copy.h"

int directory(char* directName) { // source directory->directName
    int flag = 0;
    DIR* pDir = NULL;
    struct dirent* pDirent;
    char path[PATH_MAX];
    char pathd[PATH_MAX];

    if ((pDir = opendir(directName)) == NULL) { // open direction
        perror("Directory couldn't be opened.");
        return -1;
    }

    while ((pDirent = readdir(pDir)) != NULL) { // open , read and copy files in the direction
        if (strcmp(pDirent->d_name, ".") != 0 && strcmp(pDirent->d_name, "..") != 0 && pDirent->d_name[strlen(pDirent->d_name) - 1] != '~') {
            strcpy(path, directName);
            strcat(path, "/");
            strcat(path, pDirent->d_name);

            if (is_directory(path) == 1 || is_nonRegular(path) == 1) {
                directory(path);
            }
            else {
                strcpy(pathd, destArray);
                strcat(pathd, "/");
                strcat(pathd, pDirent->d_name);

                if (copy_files(path, pathd) != 0) { // copy
                    fprintf(stderr, "Failed to copy file from %s to %s\n", path, pathd);
                }
            }
        }
    }

    while ((closedir(pDir) == -1) && (errno == EINTR))
        ;
    return 1;
}

int is_nonRegular(char* Path) {
    struct stat statbuf;
    lstat(Path, &statbuf);
    // file is regular or not
    // regular means for example it is fifo file
    if (S_ISBLK(statbuf.st_mode) || S_ISCHR(statbuf.st_mode) || S_ISFIFO(statbuf.st_mode) || S_ISLNK(statbuf.st_mode) || S_ISSOCK(statbuf.st_mode)) {
        return 1;
    }
    return 0;
}

int is_directory(char* Path) { // is this path is directory, if it is not it create an error
    struct stat statbuf;
    lstat(Path, &statbuf);

    if (S_ISDIR(statbuf.st_mode)) {
        return 1;
    }
    return 0;
}
