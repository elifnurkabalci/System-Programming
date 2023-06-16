#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <dirent.h>
#include <signal.h>

char * serverPath;
struct sockaddr_in address;
char * connectedPath[128] = {0};
int connectedClients[128] = {0};

int totalClientsConnected=0;
int connectionIdAI=0;
int threadPoolSize = 4;

pthread_t * threads;
int server_fd;

char * complete_string (char * c1,char * c2,char * c3){
    char *arr = (char*) malloc(255);//Allocate memory in 255 size to hold result string
    snprintf(arr, 255, "%s%s%s", c1, c2, c3); // arr = c1 + c2 + c3 
    return arr;
}
void removeDirectory(char * path){
    DIR *dir = opendir(path); // check all files and down to top delete files
    struct dirent *ent;

    while ((ent = readdir(dir)) != NULL) {
        if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
            continue;

        char *pathall = complete_string(path, "/",ent->d_name);

        if(ent->d_type==4){
            removeDirectory(pathall);
            continue;
        }

        unlink(pathall); // delete this line
        free(pathall);
    }
    rmdir(path);
    closedir(dir);
}
void sendMissingFiles(int fd,char * path,FILE * logFile) {
    char * serverClientPath = complete_string(serverPath,"/",path);
    char readBuffer[1024];
    char sendBuffer[1024];
    DIR * dirr = opendir(serverClientPath);

    if(dirr==NULL)return ;

    struct dirent *ent;

    while ((ent=readdir(dirr)) != NULL){
        if(ent->d_name[0] == '.')//Skip files with . and ..
            continue;
        if(strcmp(ent->d_name,"server.log")==0){
            continue;
        }
        //get relative path to file or directory
        char * pathall = complete_string(path,"/",ent->d_name);
        char * pathall2 = complete_string(serverClientPath,"/",ent->d_name);
        if(ent->d_type==8){
            struct stat st = {0};
            stat(pathall2, &st);
            sprintf(sendBuffer,"FILE\n%s\n%ld",pathall,st.st_size);
            send(fd,sendBuffer, strlen(sendBuffer)+1,0);
            recv(fd,readBuffer, 1024,0);
            readBuffer[3]=0;

            if(strcmp(readBuffer,"yes")==0){ 
                FILE *fp = fopen(pathall2, "rb");
                size_t bytesRead;
                size_t totalBytesRead = 0;
                while( (bytesRead = fread(sendBuffer, 1, sizeof(sendBuffer), fp))>0 ){
                    totalBytesRead += bytesRead;
                    send(fd, sendBuffer, bytesRead, 0);
                }
                fclose(fp);
                fprintf(stderr,"The file %s sent to client successfully\n",pathall);
                fprintf(logFile,"The file %s was sent to client\n",pathall);
                fflush(logFile);
                if(recv( fd , readBuffer, 1024,0)>0){ 
                    char * pathallMod = complete_string(serverClientPath,"/.last_mod_",ent->d_name);
                    FILE* fp2 = fopen( pathallMod, "w+");
                    fwrite(readBuffer,1,strlen(readBuffer),fp2);
                    fclose(fp2);
                }
            }
        }else if(ent->d_type==4){ 
            sprintf(sendBuffer,"DIR\n%s",pathall);
            send(fd,sendBuffer, strlen(sendBuffer)+1,0);
            recv(fd,readBuffer, 1024,0);
            sendMissingFiles(fd,pathall,logFile);
        }
        free(pathall);
    }

    free(serverClientPath);
    closedir(dirr);//free dirr allocated in opendir

}
static void createClientDirectory(char *path) { // create directory for reach client
    char arr[256];
    char *i = NULL;
    size_t len;

    snprintf(arr, sizeof(arr),"%s",path);
    len = strlen(arr);
    if(arr[len - 1] == '/')
        arr[len - 1] = 0;
    for(i = arr + 1; *i; i++)
        if(*i == '/') {
            *i = 0;
            mkdir(arr, S_IRWXU);
            *i = '/';
        }
    mkdir(arr, S_IRWXU);
}
void * threadCreate(void * vargp){
    int clientId = 0;
    char readBuffer[1024]; // server-> client
    char sendBuffer[1024]; // client->server

    int serverFd = *((int*)vargp);
    int socketFd;
    clientId = connectionIdAI++;
    char command[32];
    char path[128];
    char filename[128];
    char filesize[32];
    char modified[32];

    while(1){
        int addrlen = sizeof(address);
        if ((socketFd = accept(serverFd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0){
            printf("accept");
            fflush(stdout);
            exit(EXIT_FAILURE);
        }
        if(totalClientsConnected==threadPoolSize){ // if thread pool size and total client connected are same, total client is full
            send(socketFd , "max" , 4 , 0 );
            printf("FULL Exit socketThread with %d \n",socketFd);
            continue;
        }
        send(socketFd , "yes" , 4 , 0 ); // if totalclient<threadpool , say client ieverything is good
        totalClientsConnected++;
        fflush(stdout);
        recv(socketFd,path,128,0);
        int i;
        for(i=0;i<threadPoolSize+1;i++){
            if(strcmp(connectedPath[i],path)==0){
                send(socketFd , "no" , 3 , 0 );
                continue;
            }
        }
        send(socketFd , "yes" , 4 , 0 );  // if connectedpath is not full
        strcpy(connectedPath[clientId],path); // add connected path to client number
        connectedClients[clientId] = socketFd;

        char * serverClientPath = complete_string(serverPath,"","");
        char * logPath = complete_string(serverClientPath,"/","server.log"); // server log
        free(serverClientPath);

        FILE* logFile = fopen( logPath, "w+"); 
        fprintf(logFile,"Client was connected to server with path %s\n",path);
        fflush(logFile);
        free(logPath);
        printf("Connection %d accepted with path %s\n",clientId, connectedPath[clientId]);

        while (read( socketFd , readBuffer, 1024)>0){ // read client's response 

            sscanf(readBuffer,"%[^\t\n]\n%[^\t\n]\n%[^\t\n]\n%[^\t\n]\n%[^\t\n]",command,path,filename,filesize,modified);

            serverClientPath = complete_string(serverPath,"","");

            createClientDirectory(serverClientPath);
            struct stat st = {0};

            if(strcmp(command,"FILE")==0){ // for creation file
                int totalFileSize = atoi(filesize);

                if(strcmp(filename,"server.log")==0){
                    send(socketFd , "no" , 3 , 0 );
                    continue;
                }
                char * pathall = complete_string(serverClientPath,"/",filename);

                char * pathallHidden = complete_string(serverClientPath,"/.last_mod_",filename);

                FILE* checkFile = fopen( pathallHidden, "r");
                if(checkFile!=NULL){
                    fread(readBuffer,1, 10,checkFile);
                    readBuffer[10]=0;

                    if(strcmp(modified,readBuffer)==0){ //check modified date
                        stat(pathall, &st);
                        if(st.st_size==totalFileSize){ //check file size
                            send(socketFd , "no" , 3 , 0 );
                            continue;
                        }
                    }
                    fclose(checkFile);
                }
                FILE* fp = fopen( pathall, "wb");

                FILE* fp2 = fopen( pathallHidden, "w+");
                fwrite(modified,1,strlen(modified),fp2);
                fclose(fp2);
                free(pathallHidden);

                if(totalFileSize==0){ // if file is empty directly copy only file
                    send(socketFd , "empty" , 6 , 0 );
                }else{ // if it is not copy ingredient also
                    send(socketFd , "yes" , 4 , 0 );
                    int tot=0,b;
                    if(fp != NULL) {
                        while ((b = recv(socketFd, readBuffer, 1024, 0)) > 0) {
                            tot += b;
                            fwrite(readBuffer, 1, b, fp);
                            if(tot==totalFileSize){
                                break;
                            }
                        }
                        sprintf(sendBuffer,"i have received");
                        fprintf(stderr,"I have updated the file %s\n",pathall);
                        send(socketFd , sendBuffer , strlen(sendBuffer)+1 , 0 );
                        if (b < 0)
                            perror("Receiving");

                        fclose(fp);
                    }
                }
                fprintf(logFile,"File %s was updated\n",pathall);
                fflush(logFile);
                free(pathall);
            }else if(strcmp(command,"FILEDELETE")==0) { // for deletion file
                char * pathall2 = complete_string(serverClientPath,"/.last_mod_",filename);
                char * pathall = complete_string(serverClientPath,"/",filename);
                unlink(pathall2); // unlink the pointer of the path
                if(unlink(pathall)<0){
                    send(socketFd , "no" , 3 , 0 );
                }else{
                    send(socketFd , "yes" , 4 , 0 );
                    fprintf(stderr,"I have deleted the file %s\n",pathall);
                }

                fprintf(logFile,"File %s was deleted\n",pathall);
                fflush(logFile);
                free(pathall);
                free(pathall2);
            }else if(strcmp(command,"FOLDER")==0) { // create folder
                if (stat(serverClientPath, &st) == -1) {
                    mkdir(serverClientPath, 0700);
                    fprintf(stderr,"creating folder %s\n",serverClientPath);
                    fprintf(logFile,"Folder %s was created\n",serverClientPath);
                    fflush(logFile);
                    send(socketFd , "yes" , 4 , 0 );
                }else{
                    send(socketFd , "no" , 3 , 0 );
                }
            }else if(strcmp(command,"FOLDERDELETE")==0) { // delete folder
                sprintf(sendBuffer,"I am deleting folder");
                removeDirectory(serverClientPath);
                fprintf(stderr,"Folder deleted %s\n",serverClientPath);
                fprintf(logFile,"Folder %s was deleted\n",serverClientPath);
                fflush(logFile);
                send(socketFd , sendBuffer , strlen(sendBuffer)+1 , 0 );
            }else if(strcmp(command,"GETMISSINGFILES")==0) { // check files
                sprintf(sendBuffer,"I am deleting folder");
                sendMissingFiles(socketFd,path,logFile);
                sprintf(sendBuffer,"EXIT\n%s",path);
                send(socketFd,sendBuffer, strlen(sendBuffer)+1,0);
            }else{
                send(socketFd , "OK NOT" , strlen("OK NOT")+1 , 0 );
            }
            free(serverClientPath);
        }
        connectedPath[clientId][0] = 0;
        printf("Connection %d was closed\n",clientId);
        close(socketFd);
        fclose(logFile);
        connectedClients[clientId] = 0;

        totalClientsConnected--;
    }
    
    pthread_exit(NULL);
    return NULL;
}
void signalHandler(int sig){
    close(server_fd);
    int i;
    for(i=0;i<threadPoolSize+1;i++){ // close and free pointer for protect form garbage
        if(connectedPath[i]!=NULL){
            free(connectedPath[i]);
        }
    }

    for(i=0;i<threadPoolSize+1;i++){
        if(connectedClients[i]!=0){
            close(connectedClients[i]);
        }
    }
free(serverPath);
    printf("\nExiting BibakBOXServer...\n\n");
    exit(0);
}
int main(int argc, char const *argv[]){
    if(argc<4){ // parameter chack
        printf("BibakBOXServer [directory] [threadPoolSize] [portnumber] \n");
        exit(EXIT_FAILURE);
    }

    serverPath = malloc(128); // parameter assign
    strcpy(serverPath,argv[1]);
    threadPoolSize = atoi(argv[2]);
    int port = atoi(argv[3]);
    
    int i;
    for(i=0;i<threadPoolSize+1;i++){ // allocate memory for 
        connectedPath[i] = calloc(255, sizeof(char));
    }
    if (signal(SIGINT, signalHandler) == SIG_ERR) // general signal handler capturer
        printf("\nCan't catch SIGINT\n");

    if (signal(SIGTERM, signalHandler) == SIG_ERR)
        printf("\nCan't catch SIGTERM\n");

    if (signal(SIGQUIT, signalHandler) == SIG_ERR)
        printf("\nCan't catch SIGQUIT\n");


    int op = 1;
    struct stat st = {0};
    if (stat(serverPath, &st) == -1) { // if does not exist server workplace, create
        mkdir(serverPath, 0700);
    }

    // create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){ 
        printf("socket failed");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &op, sizeof(op))){
        printf("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET; // define adresss server and clients are same
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( port );
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0){
        printf("bind failed");
        fflush(stdout);
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 0) < 0){ // if socket cant listen server client file direction
        printf("listen");
        fflush(stdout);
        exit(EXIT_FAILURE);
    }
    printf("Server listening on %d\n",port); 
    fflush(stdout);
    threads = malloc(threadPoolSize* sizeof(pthread_t));

    for(i=0;i<threadPoolSize+1;i++){ // create thread
        pthread_create(&(threads[i]), NULL, threadCreate, &server_fd);
    }

    for(i=0;i<threadPoolSize+1;i++){
        pthread_join(threads[i],NULL);
    }


    return 0;
}
