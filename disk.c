#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include "disk.h"
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include "ifs_structs.h"
#include <math.h>

#define BLOCKSIZE 1024

struct disk{
    int fd;
    int blockCount;
};

//create a virtual disk and set the fd to -1
struct disk _disk = {.fd=-1 };

int min(int x, int y){
    if (x < y){
        return x;
    }
    else if (x> y){
        return y;
    }
    else{
        return x; 
    }
}
int createDisk(const char *disk, int bcount){
    int  fd;

    if (!disk){
        fprintf(stderr, "Error: %s", strerror(errno));
        return -1;
    }
    //create and open virtual disk lol 
    if ((fd = open(disk, O_RDWR| O_CREAT|O_TRUNC, 0644)) < 0){
        fprintf(stderr, "Error: %s", strerror(errno));
        return -1;
    }
    if (ftruncate(fd, bcount * BLOCKSIZE) < 0){
        fprintf(stderr, "Error: %s", strerror(errno));
        return -1;
    }
    close(fd);
    return 0;
}

int openDisk(const char *disk){
    int fd; 
    struct stat st;

    if (!disk){
        fprintf(stderr, "Error: %s", strerror(errno));
        return -1;
    }
    if(_disk.fd != -1){
        fprintf(stderr, "Error: Disk already open");
        return -1;
    }
    if ((fd = open(disk, O_RDWR, 0644)) < 0){
        fprintf(stderr, "Error with open");
        return -1;
    }
    if (fstat(fd, &st)){
        fprintf(stderr, "Error with stat");
        return -1;
    }
    if (st.st_size % BLOCKSIZE !=0){
        fprintf(stderr, "Error: Block size not comptabile");
        return -1;
    }
    _disk.fd = fd;
    _disk.blockCount = st.st_size/BLOCKSIZE;
    return 0;
}

int diskClose(){
    if (_disk.fd == -1){
        fprintf(stderr, "Error: Cannot close file because it doesn't exist");
        return -1;
    }
    if (close(_disk.fd) < 0){
        fprintf(stderr, "Error: Cannot close file");
        return -1;
    }
    _disk.fd = -1;
    return 0;
}

int blockWrite(int block, const void *buffer){
    if (_disk.fd == -1){
        fprintf(stderr, "Error: Cannot open file that doesnt exist");
        return -1;
    }
    if (block >= _disk.blockCount){
        fprintf(stderr, "Error: Block greater than the total block size");
        return -1;
    }
    if (lseek(_disk.fd, block * BLOCKSIZE, SEEK_SET) < 0){
        fprintf(stderr, "Error: problem with lseek");
        return -1; 
    }
    if (write(_disk.fd, buffer, BLOCKSIZE)<0){
        fprintf(stderr, "Error: write");
        return -1;
    }
    return 0;
}



int writeToFile( char *text, int inode[]){
    if (_disk.fd == -1){
        fprintf(stderr, "No disk open");
        return -1;
    }
    int size = 0; 
    char data[1024];
    int index = 0;
    char usertext[BLOCKSIZE * 10];
    strcpy(usertext, text);
    while (usertext != ""){
        int offset = -1;
        blockRead(inode[index], data);
        for(int i = 0 ; i<BLOCKSIZE; i++){
            offset += 1;
            if (data[i] == NULL){
                break;
            }
        }
        //printf("offset %d\n", offset);
        int position = BLOCKSIZE * inode[index] + offset;
        int bytesToBeWrittenInBlock = min(BLOCKSIZE - offset, strlen(usertext));
        if (index == 9){
            if (strlen(usertext) > bytesToBeWrittenInBlock){
                fprintf(stderr, "Not enough space\n");
                return -1;
            }
        }
        //printf("inode number: %d\n", inode[index]);
        //printf("position %d\n", position);
        //printf("bytes to be written %d\n", bytesToBeWrittenInBlock);
        if (lseek(_disk.fd, position, SEEK_SET) < 0){
            fprintf(stderr, "Error: lseek");
            return -1;
        }
        char currText[bytesToBeWrittenInBlock]; 
        memcpy(currText, &usertext[0], bytesToBeWrittenInBlock);
        currText[bytesToBeWrittenInBlock] = '\0';
        //printf("current text %s\n", currText);
        //printf("%s\n", currText);
        size += strlen(currText);
        if (write(_disk.fd, currText, strlen(currText)) < 0){
            fprintf(stderr, "Error: write");
            return -1;
        }

        char *restText = malloc(strlen(usertext) - bytesToBeWrittenInBlock);
        if (bytesToBeWrittenInBlock == strlen(usertext)){
            break;
        }
        
        memcpy(restText, &usertext[bytesToBeWrittenInBlock], strlen(usertext) - bytesToBeWrittenInBlock );
        strcpy(usertext, restText);
        usertext[strlen(text)] = '\0';
        //printf("ending text: %s\n", text);
        //printf("new string after: %s\n", restText);
        index++;
    }
    return size;
}
int truncateBlock(int block){
    if (_disk.fd == -1){
        fprintf(stderr, "No disk open");
        return -1;
    }
    if (lseek(_disk.fd, block * BLOCKSIZE, SEEK_SET) < 0){
        fprintf(stderr, "Error: lseek");
        return -1;
    }
    for (int i = 0; i < BLOCKSIZE ; i++){
        if (write(_disk.fd, "\0", 1) < 0){
            fprintf(stderr, "error: truncate block\n");
            return -1;
        }
    }
    return 0;
}
int blockRead(int block, void *buf){
    if (_disk.fd == -1){
        fprintf(stderr, "No disk open");
        return -1;
    }
    if (block >= _disk.blockCount){
        fprintf(stderr, "Error: Block index out of range");
        return -1;
    }
    if (lseek(_disk.fd, block*BLOCKSIZE, SEEK_SET) < 0){
        fprintf(stderr, "Error: lseek");
        return -1;
    }
    if (read(_disk.fd, buf, BLOCKSIZE) < 0){
        fprintf(stderr,"Error: read");
        return -1;
    }
    return 0;
}