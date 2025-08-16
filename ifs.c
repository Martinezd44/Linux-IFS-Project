#include "ifs_structs.h"
#include "disk.h"
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#define DISK "virtualDisk"
#define SUPERBLOCK 0
#define INODELIST 1
#define BITMAP 2
#define USED '1'
#define REGULARFILE 10000
#define DIRECTORYFILE 40000
#define UNUSED '0'
#define MAXFILES 1024
#define ROOT 3
struct ifs_super_block sb;
struct inode *inodes;

void usageMessage(){
    printf("Usage: ./ifs -u <userID> -f <drive>\n");
}

int findFreeInode(){
    char data[1024];
    blockRead(BITMAP,data);
    for (int i = 0; i < strlen(data);i++){
        if(data[i] == UNUSED){
            data[i] = USED;
            blockWrite(BITMAP, data);
            return i;
        }
    }
    return -1;
}

int findFreeInodeInIL(){
    struct inode inodes[1024];
    struct ifs_super_block isb[2];
    blockRead(SUPERBLOCK, isb);
    blockRead(INODELIST, inodes);
    char data[1024];
    blockRead(BITMAP,data);
    for (int i = 0; i < isb[0].numberOfFiles;i++){
        if(data[i] == UNUSED){
            data[i] = USED;
            blockWrite(BITMAP, data);
            return i;
        }
    }
    return -1;
}
int createDirectory(const char *filename, int parent){
    if (!filename){
        fprintf(stderr, "Error: bad filename");
    }
    int inode = findFreeInodeInIL();
    if (inode < 0){
        fprintf(stderr, "No space\n");
        return -1; 
    }
     
    struct inode arr[2000]; 
    struct directory existingDirectories[1024];
    struct directory directory;
    char bitmap[IFS_BLOCK_SIZE];
    blockRead(INODELIST, (void*)arr);
    blockRead(BITMAP, (void*)bitmap);
    blockRead(parent, existingDirectories);
    arr[inode].i_ino = inode;
    arr[inode].i_type = DIRECTORYFILE;
    arr[inode].i_mode = 666;
    bitmap[inode] = USED;
    blockWrite(BITMAP, bitmap);
    blockWrite(INODELIST, arr);
    directory.d_dinode = inode; 
    strcpy(directory.d_dname, filename);
    bitmap[inode] = USED;
    blockWrite(BITMAP, bitmap);
    for (int i = 0; i < 128;i++){
        if (existingDirectories[i].d_dinode == 0){
            existingDirectories[i] = directory;
            inode = i;
            break;
        }
    }
    blockWrite(parent, existingDirectories);
}

void createSuperBlock(){
    double i =  pow(2.0,floor(log2((1468006.4 - (2 * IFS_BLOCK_SIZE))/IFS_BLOCK_SIZE)));
    sb.totalNumOfBlocks = (int)i;
    sb.numberOfFiles = (int)IFS_BLOCK_SIZE/sizeof(struct inode);
    //total number of blocks is inode blocks plus 2 (because of super block and inode list)
    sb.num_blocks = (int)i + 2;
    sb.root_inode = 1;
    blockWrite(SUPERBLOCK, &sb);
}
//initialize the file system with empty values 
void create_fs(int userID){
    char bitmap[IFS_BLOCK_SIZE + 1] = "";
    struct ifs_super_block isb[2];
    blockRead(SUPERBLOCK, isb);
    for (int i = 0; i < isb[0].num_blocks + 1;i++){
        if (i == SUPERBLOCK || i == INODELIST || i == BITMAP){
            strcat(bitmap, "1");
        }
        else{
            strcat(bitmap, "0");
        }
    }
    inodes = malloc(IFS_BLOCK_SIZE);
    int ino = 20;
    for(int i =0; i<isb[0].numberOfFiles;i++){
        inodes[i].i_ino = i;
        inodes[i].i_type = -1;
        inodes[i].i_mode = -1;
        inodes[i].i_uid = userID;
        for (int j = 0; j < 10; j++){
            inodes[i].freeInodes[j] = ino;
            bitmap[inodes[i].freeInodes[j]] = USED;
            ino +=1;  
        }
        inodes[i].i_gid=-1;
        inodes[i].i_size=-1;
        inodes[i].i_accesstime=-1;
        inodes[i].i_changetime=-1;  
    }
    blockWrite(INODELIST, inodes);
    blockWrite(SUPERBLOCK, &sb);
    bitmap[3]= USED;
    blockWrite(BITMAP, bitmap);
    blockRead(BITMAP, bitmap);
    blockRead(INODELIST, inodes);
    blockRead(BITMAP, bitmap);
    diskClose();
}

int createFile(const char *filename, int parent){
    if (!filename){
        fprintf(stderr, "Error: bad filename");
        return -1;
    }
    int inode = findFreeInodeInIL();
    int returnInode = inode;
    if (inode < 0){
        fprintf(stderr, "Not enough space");
        return -1;
    }
    struct inode arr[2000]; 
    char bitmap[IFS_BLOCK_SIZE];
    blockRead(BITMAP, (void*)bitmap); 
    blockRead(INODELIST, (void*)arr );
    arr[inode].i_type = REGULARFILE;
    arr[inode].i_mode=666;
    arr[inode].i_gid = 2000;
    arr[inode].i_size = 0;
    bitmap[inode] = USED;
    bitmap[parent] = USED;
    blockWrite(BITMAP, bitmap);
    blockWrite(INODELIST, arr);
    struct directory entry;
    entry.d_dinode = inode;
    strcpy(entry.d_dname, filename);
    struct directory existingDirectories[1024];
    blockRead(parent, (void*)existingDirectories);
    for (int i = 0; i < 128;i++){
        if (existingDirectories[i].d_dinode == 0){
            existingDirectories[i] = entry;
            inode = i;
            break;
        }
    }
    blockWrite(parent, existingDirectories);
    return returnInode;
    //blockWrite(parent, entry);
}  


int appendToFile(int inode, char *text){
    struct inode arr[2000]; 
    blockRead(INODELIST, (void*)arr);
    struct ifs_super_block isb[2];
    blockRead(SUPERBLOCK, isb);
    int bytes;
    for (int i = 0; i < isb[0].numberOfFiles ; i++){
        if (arr[i].i_ino == inode){
            if(bytes = writeToFile(text, arr[i].freeInodes) < 0){
                return -1;
            }
        }
    }
    return bytes; 
}

int truncateFile(int inode){
    struct inode arr[2000]; 
    blockRead(INODELIST, (void*)arr );
    struct ifs_super_block isb[2];
    blockRead(SUPERBLOCK, isb);
    for (int i = 0; i < isb[0].numberOfFiles ; i++){
        if (arr[i].i_ino == inode){
            for (int j = 0; j < 10; j++){
                if (truncateBlock(arr[i].freeInodes[j]) < 0){
                    return -1;
                }
            }
        }
    }
    return 0;
}

int printFile(int inode){
    struct inode arr[2000]; 
    char data[IFS_BLOCK_SIZE];
    blockRead(INODELIST, arr );
    struct ifs_super_block isb[2];
    blockRead(SUPERBLOCK, isb);
    for (int i = 0; i<isb[0].numberOfFiles; i++){
        if (arr[i].i_ino == inode){
            if (arr[i].i_type == DIRECTORYFILE){
                printf("File not found");
                return -1;
            }
            for (int j = 0; j < 10; j++){
                blockRead(arr[i].freeInodes[j], data);
                for (int k=0;k<IFS_BLOCK_SIZE; k++){
                    if (data[k] != NULL){
                    printf("%c", data[k]);
                    }
                }
            }
            return 0;
        }
    }
    return 0; 
}
    //compile file ising gcc -Wall ifs.c disk.c -o ./a.out
    


/*
To-Do:c
implement inode table 
1. write to a file (append and replace)
2. user login
3. get opts 
4. commands such as ls, ll, cd, touch, rm, mkdir, stat, tree, pwd, whoami, and cat (reading)
5. directories lol 
6. minishell
*/