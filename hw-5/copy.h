#ifndef COPY_H
#define COPY_H

int copy_files(char* inFile, char* outFile);

extern int numcopiers;
extern int bufsize;
extern char destArray[100];
extern int doneflag;

#endif /* COPY_H */
