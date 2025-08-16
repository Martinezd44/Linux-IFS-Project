#include "ifs.h"
#include "disk.h"
#include "ifs_structs.h"
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#define BLUE "\x1b[34;1m"
#define DEFAULT "\x1b[0m"
#define GREEN   "\033[32m" 
#define SUPERBLOCK 0
#define INODELIST 1
#define BITMAP 2
#define USED '1'
#define REGULARFILE 10000
#define DIRECTORYFILE 40000
#define UNUSED '0'
#define IFS_BLOCK_SIZE 1024
#define ROOT 3
struct ifs_super_block sb;

int displayStat(int parentInode, char *filename){
    struct inode arr[2000];
    struct ifs_super_block isb[2];
    struct directory entries[32];
    blockRead(SUPERBLOCK, isb);
    blockRead(INODELIST, arr);
    int flag = 0;
    int inode = 0; 
    blockRead(parentInode, entries); 
    struct tm * timeinfo;
    for (int i = 0 ; i< 16; i++){
        if (strcmp(entries[i].d_dname, filename)==0){
            inode = entries[i].d_dinode;
            flag = 1;
         }
    }
    
    if (flag == 0){
        return -1;
    }
    for (int i = 0 ; i < isb[0].numberOfFiles; i ++){
        
        if (arr[i].i_ino == inode){
            printf("File: %s\n",filename);
            char fileType[128]; 
            if (arr[i].i_type == DIRECTORYFILE){
                strcpy(fileType, "Directory File");
            }
            else{
                strcpy(fileType, "Regular File");
            }
            printf("Size: %d bytes   Blocks: 10    File Type: %s\n", arr[i].i_size, fileType);
            printf("Inode: %d\n", i);
            printf("Access: (%d)    UID: %d    GID: %d\n", arr[i].i_mode, arr[i].i_uid, arr[i].i_gid);
            timeinfo = localtime(&arr[i].i_accesstime);
            printf("Access: %s",asctime(timeinfo));
            timeinfo = localtime(&arr[i].i_changetime);
            printf("Modify: %s",asctime(timeinfo));
            flag = 1;
            return 0;
        }
    }
    if (flag == 0){
        return -1;
    }
    return -1;
}
int ls(int inode, const char *disk){
    struct directory directories[1024];
    struct inode arr[2000];
    blockRead(inode, directories);
    blockRead(INODELIST,arr);
    for (int i = 0; i < 32; i++){
        if (directories[i].d_dinode != 0){
            if (arr[directories[i].d_dinode].i_type == DIRECTORYFILE){
                printf("%s%s%s\n", GREEN, directories[i].d_dname, DEFAULT);
            }
            else{
                printf("%s\n",  directories[i].d_dname);
            }
            
        }
    }
    return 0;
}


int mkdir(const char* filename, int parent ){
    createDirectory( filename, parent );
    return 0;
}
int removeFile(int parentInode, char *fileToBeRemoved){
    char data[2000];
    struct directory entries[2000];
    blockRead(parentInode, entries); 
    blockRead(BITMAP,data);
    int flag = 0;
    for (int i = 0 ; i< 16; i++){
        if (strcmp(entries[i].d_dname, fileToBeRemoved)==0){
            
            truncateFile(entries[i].d_dinode);
            data[entries[i].d_dinode] = UNUSED;
            entries[i].d_dinode = 0;
            strcpy(entries[i].d_dname, "");
            flag = 1;
            break;
         }
    }
    if (flag == 0){
        printf("File not found\n");
        return -1;
    }
    blockWrite(parentInode, entries);
    blockWrite(BITMAP, data);
    return 0;

}


char* printcurrwd(int startInode, int endInode, char *path){
    struct directory directories[1024];
    blockRead(startInode, directories);
    char possiblePath[128]= "/";
    int count = 0;
    char *help = malloc(128);
    while(directories[count].d_dinode != 0){
        count++;
    }
    
    for (int i = 0; i < count;i++){
        sprintf(possiblePath, "%s/%s", path, directories[i].d_dname);
        if (directories[i].d_dinode == endInode){
            strcpy(help, possiblePath);
             printf("%s\n",help);
            return help;
        }
        printcurrwd(directories[i].d_dinode, endInode, possiblePath);
    }
    return NULL;
}
char* getcurrwd(int startInode, int endInode, char *path){
    struct directory directories[1024];
    blockRead(startInode, directories);
    char possiblePath[128]= "/";
    int count = 0;
    char *help = malloc(128);
    while(directories[count].d_dinode != 0){
        count++;
    }
    
    for (int i = 0; i < count;i++){
        sprintf(possiblePath, "%s/%s", path, directories[i].d_dname);
        if (directories[i].d_dinode == endInode){
            strcpy(help, possiblePath);
             printf("%s[%s]>%s",BLUE, help, DEFAULT);
            return help;
        }
        getcurrwd(directories[i].d_dinode, endInode, possiblePath);
    }
    return NULL;
}

int appendEndFile(int inode[], char *text){
    return writeToFile(text, inode);

}

int pathToInode(int length, char *pathBrokenUp[]){
    int inode = ROOT;
    struct directory entries[1024];
    int count = 0;
    int found;
    while (length != count){
        blockRead(inode, entries);
        found = 0;
        for (int i = 0; i < 32; i++){
            if (strcmp(entries[i].d_dname, pathBrokenUp[count])==0){
                inode = entries[i].d_dinode;
                count++;
                found = 1;
                break; 
            }
        }
        if (found == 0){
            return -1;
        }
    }
    return inode;
}


int main(int argc, char *argv[]){
    int opt;
    time_t seconds; 
    int fflag = 0;
    int uflag = 0;
    int userID;
    char *disk;
    while ((opt = getopt(argc, argv, "f:u:")) != -1) {
        switch (opt) {
        case 'u':
            fflag += 1;
            userID = atoi(optarg);
            break;
        case 'f':
            uflag += 1;
            disk = optarg;
            break;
        }
    }

    int currentInode = ROOT;
    if (open(disk, O_RDONLY) < 0){
        createDisk(disk, pow(2.0,floor(log2((1468006.4 - (2 * IFS_BLOCK_SIZE))/IFS_BLOCK_SIZE))));
        openDisk(disk);
        createSuperBlock();
        //openDisk(disk);
        create_fs(userID);

    }
    
    while(1){
        openDisk(disk);
        char* input[1024];
        char* args[1024];
        struct ifs_super_block isb[2];
        int count = 0;
        struct inode arr[2000];
        char* command;
        const char delim[2] = " ";
        const char delimSlash[2] = "/";
        char* token;
        char cmdline[1024];
        if (currentInode == ROOT){
             printf("%s[/]>%s",BLUE, DEFAULT);
        }
        else{
            getcurrwd(ROOT, currentInode, "");
        }
        diskClose();
        //printf("%d", currentInode);
        fgets(cmdline,1024, stdin);
        cmdline[strlen(cmdline) -1] = '\0';
        //break up the commands into smaller arrays for feasability 
        token = strtok(cmdline, delim);
        //make this variable command equal to the first parameter aka the commands like ls, echo, etc
        command = token;
        //continue looping through the given string in stdin and adding it to input array 
        while (token != NULL){
            input[count] = token;
            count++;
            token = strtok(NULL, delim);
        }
        //strip any /n characters that will interfere with next part
        char* cmd; 
        int k = 0;
        cmd = strtok(command, "\n");
        //array of just arguments 
        for (int i = 1; i < count; i++){
            args[k] = input[i];
            k++;
        }
        int pathDepth = 0;
        char *splitPath[64] = {0};
        char string[64];
        if (args[0] != NULL){
            strcpy(string, args[0]);
            char *split = strtok(string, delimSlash);
            while(split != NULL){
                splitPath[pathDepth] = split;
                pathDepth++;
                split = strtok(NULL, delimSlash);
            }
        }
        if (strcmp(cmd, "ls") == 0){
            openDisk(disk);
            ls(currentInode, disk);
            diskClose();

        }
        else if (strcmp(cmd, "cd") ==0){

            //printf("%d", currentInode);
            openDisk(disk);
            int newInode = pathToInode(pathDepth, splitPath);
            if (newInode > 0){
                struct inode check[2000];
                struct ifs_super_block isb[2];
                blockRead(INODELIST, check);
                blockRead(SUPERBLOCK, isb);
                for (int i = 0; i < isb[0].numberOfFiles;i++){
                    if (check[i].i_ino == newInode){
                        if (check[newInode].i_type == REGULARFILE){
                            printf("Directory not found\n");
                            break;
                        }
                        else{
                            currentInode = newInode;
                        }
                    }
                }
            
            }
            else{
                    printf("Directory not found\n");
                }
            memset(splitPath, 0,sizeof(splitPath));
            diskClose();
        }
        else if (strcmp(cmd, "mkdir") == 0){
            openDisk(disk);
            mkdir(args[0], currentInode);
            diskClose();
        }
        else if (strcmp(cmd, "pwd") == 0){
            openDisk(disk);
            printcurrwd(ROOT, currentInode, "") ;
            diskClose();
        }
        else if (strcmp(cmd, "touch") == 0){
            openDisk(disk);
            int fileIno = createFile(args[0], currentInode);
            blockRead(INODELIST, arr);
            blockRead(SUPERBLOCK, isb);
            for (int i = 0; i < isb[0].numberOfFiles; i++){
                if (arr[i].i_ino == fileIno){
                    time(&seconds);
                    arr[fileIno].i_accesstime = seconds;
                    arr[fileIno].i_changetime = seconds;
                }
            }
            blockWrite(INODELIST, arr);
            blockRead(INODELIST, arr);
            diskClose();
        }
        
        else if (strcmp(cmd, "whoami") == 0){
            printf("%d\n", userID);
        }
        
        else if (strcmp(cmd, "write") == 0){
            char* mode;
            if (input[2] == NULL){
                printf("Usage: write <filepath> [-t|-c|-a]");
            }
            else{
                mode = strtok(input[2], "\n");
            }
            openDisk(disk);
            
            if (strcmp(mode, "-a") == 0){
                int fileIno = pathToInode(pathDepth, splitPath);
                if (fileIno < 0){
                    printf("File not found");
                }
                else{
                printFile(fileIno);
                while (1){
                    char text[1024];
                    printf("%s>>>%s",BLUE, DEFAULT);
                    fgets(text,1024, stdin);
                    if (strcmp(text, "quit\n") == 0){
                        break;
                    }
                    int bytes = -1;
                    blockRead(SUPERBLOCK, isb);
                    blockRead(INODELIST, arr);
                    for (int i = 0; i < isb[0].numberOfFiles;i++){
                        if (arr[i].i_ino == fileIno){
                             bytes = appendEndFile(arr[i].freeInodes, text);
                        }
                    }
                    for (int i = 0; i < isb[0].numberOfFiles; i++){
                        if (arr[i].i_ino == fileIno){
                            time(&seconds);
                            arr[i].i_changetime = seconds;
                            arr[i].i_size+=bytes;
                        }
                    }
                    blockWrite(INODELIST, arr);
                        }
                }
                

            }
            if (strcmp(mode, "-t") == 0){
                int fileIno = pathToInode(pathDepth, splitPath);
                truncateFile(fileIno);
                blockRead(INODELIST,arr);
                for (int i = 0; i < isb[0].numberOfFiles; i++){
                    if (arr[i].i_ino == fileIno){
                        arr[i].i_size = 0;
                    }
                }
                blockWrite(INODELIST, arr);
            }
            if (strcmp(mode, "-c") == 0){
                //get path of parent 
                //get inode of parent 
                //create file in that parent 
                char *newFileName = splitPath[pathDepth-1];
                printf("%s", newFileName);
                int fileIno = 0;
                if (pathDepth == 1){
                    fileIno = createFile(newFileName, ROOT);
                }
                else{
                    //splitPath[pathDepth-1] = NULL;
                    int parentInode = pathToInode(pathDepth - 1, splitPath);
                    fileIno = createFile(newFileName, parentInode);
                }
                blockRead(INODELIST, arr);
                for (int i = 0; i < isb[0].numberOfFiles; i++){
                        if (arr[i].i_ino == fileIno){
                            time(&seconds);
                            arr[i].i_accesstime = seconds;
                        }
                    }
                    blockWrite(INODELIST, arr);
                while(1){
                    char text[1024];
                    printf("%s>>>%s",BLUE, DEFAULT);
                    fgets(text,1024, stdin);
                    if (strcmp(text, "quit\n") == 0){
                        break;
                    }
                    blockRead(SUPERBLOCK, isb);
                    blockRead(INODELIST, arr);
                    for (int i = 0; i < isb[0].numberOfFiles;i++){
                        if (arr[i].i_ino == fileIno){
                            appendEndFile(arr[i].freeInodes, text);
                        }
                    }
                    blockRead(INODELIST, arr);
                     for (int i = 0; i < isb[0].numberOfFiles; i++){
                        if (arr[i].i_ino == fileIno){
                            time(&seconds);
                            arr[i].i_changetime = seconds;
                        }
                    }
                    blockWrite(INODELIST, arr);
                }
            }
            diskClose();
        }

        else if (strcmp(cmd, "cat") == 0){
            openDisk(disk);
            int fileIno = pathToInode(pathDepth, splitPath);
            printFile(fileIno);
            blockRead(INODELIST, arr);
            for (int i = 0; i < isb[0].numberOfFiles; i++){
                if (arr[i].i_ino == fileIno){
                    time(&seconds);
                    arr[i].i_accesstime = seconds;
                }
            }
            blockWrite(INODELIST, arr);
            diskClose();
        }

        else if (strcmp(cmd, "rm") == 0){
            if (input[1] == NULL){
                printf("File not found\n");
            }
            else{
                openDisk(disk);
                removeFile(currentInode, input[1]);
                diskClose();
            }
        }
        else if(strcmp(cmd, "stat") == 0){
            if (input[1] == NULL){
                printf("File not found\n");
            }
            
            else{
                openDisk(disk);
                displayStat(currentInode, input[1]);
                diskClose();
            }
        }
        else{
            printf("Command not found\n");
        }
    }
    diskClose(); 
}
    
